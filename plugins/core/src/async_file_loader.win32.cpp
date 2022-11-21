// Copyright Chad Engler

#include "he/core/async_file_loader.h"

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/result.h"
#include "he/core/scope_guard.h"
#include "he/core/sync.h"
#include "he/core/thread.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"

#include "blockingconcurrentqueue.h"

#include <atomic>

#if defined(HE_PLATFORM_API_WIN32)

#include "file_helpers.win32.h"

#include "he/core/win32_min.h"

#include "dstorage.h"

namespace he
{
    static Mutex s_initMutex{};
    static uint32_t s_initCount{ 0 };
    static IDStorageFactory* s_factory{ nullptr };
    static HANDLE s_callbackThread{ nullptr };
    static std::atomic<bool> s_callbackRunning{ true };

    struct CallbackEntry
    {
        AsyncFileQueue::LoadDelegate callback;
        IDStorageStatusArray* status;
        uint32_t index;
    };
    static moodycamel::BlockingConcurrentQueue<CallbackEntry> s_callbackQueue{};

    static DWORD DStorageCallbackThread(LPVOID);

    DSTORAGE_COMPRESSION_FORMAT ToDsCompressionFormat(AsyncFileRequest::CompressionFormat f)
    {
        switch (f)
        {
            case AsyncFileRequest::CompressionFormat::None:
                return DSTORAGE_COMPRESSION_FORMAT_NONE;
            case AsyncFileRequest::CompressionFormat::Zlib:
                return DSTORAGE_CUSTOM_COMPRESSION_0;
        }

        HE_ASSERT(false, HE_MSG("Unknown compression format"), HE_KV(format, f));
        return DSTORAGE_COMPRESSION_FORMAT_NONE;
    }

    class AsyncFileQueueImpl final : public AsyncFileQueue
    {
    public:
        ~AsyncFileQueueImpl()
        {
            if (m_status)
            {
                m_status->Release();
                m_status = nullptr;
            }

            if (m_queue)
            {
                m_queue->Release();
                m_queue = nullptr;
            }
        }

        Result Initialize(const Config& config)
        {
            const char* name = config.name;

            m_capacity = config.capacity;

            if (!HE_VERIFY(m_capacity >= DSTORAGE_MIN_QUEUE_CAPACITY && m_capacity <= DSTORAGE_MAX_QUEUE_CAPACITY,
                HE_MSG("Queue capacity must be within DStorage limits. The value will be clamped"),
                HE_KV(capacity, m_capacity),
                HE_KV(min_capacity, DSTORAGE_MIN_QUEUE_CAPACITY),
                HE_KV(max_capacity, DSTORAGE_MAX_QUEUE_CAPACITY)))
            {
                m_capacity = Clamp<uint16_t>(m_capacity, DSTORAGE_MIN_QUEUE_CAPACITY, DSTORAGE_MAX_QUEUE_CAPACITY);
            }

            DSTORAGE_QUEUE_DESC desc{};
            desc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            desc.Capacity = config.capacity;
            desc.Priority = DSTORAGE_PRIORITY_NORMAL;
            desc.Name = name;
            desc.Device = nullptr;

            HRESULT hr = s_factory->CreateQueue(&desc, IID_PPV_ARGS(&m_queue));
            if (FAILED(hr))
                return Win32Result(hr);

            hr = s_factory->CreateStatusArray(config.capacity, name, IID_PPV_ARGS(&m_status));
            if (FAILED(hr))
                return Win32Result(hr);

            return Result::Success;
        }

        void EnqueueRequest(const AsyncFileRequest& req) override
        {
            DSTORAGE_REQUEST dsReq{};
            dsReq.Options.CompressionFormat = ToDsCompressionFormat(req.compression);
            dsReq.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            dsReq.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
            dsReq.Source.File.Source = reinterpret_cast<IDStorageFile*>(req.file.val);
            dsReq.Source.File.Offset = req.offset;
            dsReq.Source.File.Size = req.size;
            dsReq.Destination.Memory.Buffer = req.dst;
            dsReq.Destination.Memory.Size = req.dstSize;
            dsReq.UncompressedSize = dsReq.Options.CompressionFormat == DSTORAGE_COMPRESSION_FORMAT_NONE ? 0 : req.dstSize;
            dsReq.CancellationTag = req.cancellationTag;
            dsReq.Name = req.name;

            m_queue->EnqueueRequest(&dsReq);
        }

        void EnqueueDelegate(LoadDelegate callback) override
        {
            if (!callback)
                return;

            const uint32_t index = static_cast<uint32_t>(m_index++ % m_capacity);
            m_queue->EnqueueStatus(m_status, index);

            CallbackEntry cb;
            cb.callback = callback;
            cb.status = m_status;
            cb.index = index;
            s_callbackQueue.enqueue(cb);
        }

        AsyncFileTracker EnqueueTracker() override
        {
            const uint32_t index = static_cast<uint32_t>(m_index++ % m_capacity);
            m_queue->EnqueueStatus(m_status, index);
            return { index };
        }

        void CancelRequestsWithTag(uint64_t mask, uint64_t value) override
        {
            m_queue->CancelRequestsWithTag(mask, value);
        }

        void Submit() override
        {
            m_queue->Submit();
        }

        bool IsComplete(AsyncFileTracker token) const override
        {
            return m_status->IsComplete(token.val);
        }

        Result GetResult(AsyncFileTracker token) const override
        {
            const HRESULT hr = m_status->GetHResult(token.val);
            return FAILED(hr) ? Win32Result(hr) : Result::Success;
        }

    private:
        IDStorageQueue* m_queue{ nullptr };
        IDStorageStatusArray* m_status{ nullptr };
        std::atomic<uint64_t> m_index{ 0 };

        uint16_t m_capacity{ 0 };
    };

    class AsyncFileLoaderImpl final : public AsyncFileLoader
    {
    public:
        AsyncFileLoaderImpl(Allocator& allocator) : AsyncFileLoader(allocator) {}

        ~AsyncFileLoaderImpl()
        {
            m_allocator.Delete(m_defaultQueue);
            GlobalTerminate();
        }

        Result Initialize(const Config& config) override
        {
            Result r = GlobalInitialize(config);
            if (!r)
                return r;

            if (config.defaultQueue.capacity > 0)
            {
                r = CreateQueue(config.defaultQueue, m_defaultQueue);
                if (!r)
                    return r;
            }

            return Result::Success;
        }

        Result OpenFile(const char* path, AsyncFileId& fd) override
        {
            IDStorageFile* file = nullptr;
            HRESULT hr = s_factory->OpenFile(HE_TO_WCSTR(path), IID_PPV_ARGS(&file));
            fd.val = reinterpret_cast<uintptr_t>(file);
            return FAILED(hr) ? Win32Result(hr) : Result::Success;
        }

        void CloseFile(AsyncFileId fd) override
        {
            IDStorageFile* file = reinterpret_cast<IDStorageFile*>(fd.val);
            if (file)
                file->Release();
        }

        Result GetAttributes(AsyncFileId fd, FileAttributes& outAttributes)
        {
            IDStorageFile* file = reinterpret_cast<IDStorageFile*>(fd.val);

            BY_HANDLE_FILE_INFORMATION info{};
            HRESULT hr = file->GetFileInformation(&info);

            if (FAILED(hr))
                return Win32Result(hr);

            Win32ParseFileAttributes(info, outAttributes);
            return Result::Success;
        }

        AsyncFileQueue* DefaultQueue() override
        {
            return m_defaultQueue;
        }

        Result CreateQueue(const AsyncFileQueue::Config& config, AsyncFileQueue*& out) override
        {
            AsyncFileQueueImpl* queue = m_allocator.New<AsyncFileQueueImpl>();
            Result r = queue->Initialize(config);
            if (!r)
            {
                m_allocator.Delete(queue);
                queue = nullptr;
            }

            out = queue;
            return r;
        }

        void DestroyQueue(AsyncFileQueue* queue) override
        {
            if (queue != m_defaultQueue)
            {
                m_allocator.Delete(queue);
            }
        }

    private:
        Result GlobalInitialize(const Config& config)
        {
            LockGuard lock(s_initMutex);

            const uint32_t count = ++s_initCount;
            if (count > 1)
                return Result::Success;

            auto failGuard = MakeScopeGuard([&]() { UnlockedGlobalTerminate(); });

            DSTORAGE_CONFIGURATION dsConfig;
            dsConfig.NumSubmitThreads = config.submitThreadCount;
            ::DStorageSetConfiguration(&dsConfig);

            HRESULT hr = ::DStorageGetFactory(IID_PPV_ARGS(&s_factory));
            if (FAILED(hr))
                return Win32Result(hr);

            DSTORAGE_DEBUG flags = DSTORAGE_DEBUG_NONE;
        #if HE_INTERNAL_BUILD
            flags |= DSTORAGE_DEBUG_SHOW_ERRORS | DSTORAGE_DEBUG_RECORD_OBJECT_NAMES;
        #endif
        #if HE_ENABLE_ASSERTIONS
            flags |= DSTORAGE_DEBUG_BREAK_ON_ERROR;
        #endif

            s_factory->SetDebugFlags(flags);

            s_callbackRunning = true;

            s_callbackThread = ::CreateThread(nullptr, 0, DStorageCallbackThread, nullptr, 0, nullptr);
            if (s_callbackThread == nullptr)
                return Result::FromLastError();

            if (config.callbackThreadAffinity > 0)
            {
                Result affinityResult = SetThreadAffinity(reinterpret_cast<ThreadHandle>(s_callbackThread), config.callbackThreadAffinity);
                if (!affinityResult)
                    return affinityResult;
            }

            ::SetThreadDescription(s_callbackThread, L"[HE] DStorage Callback Thread");

            failGuard.Dismiss();
            return Result::Success;
        }

        void UnlockedGlobalTerminate()
        {
            HE_ASSERT(s_initCount > 0);
            const uint32_t count = --s_initCount;
            if (count > 0)
                return;

            if (s_callbackThread)
            {
                s_callbackRunning = false;
                s_callbackQueue.enqueue({});
                ::WaitForSingleObject(s_callbackThread, INFINITE);
                ::CloseHandle(s_callbackThread);
                s_callbackThread = nullptr;
            }

            if (s_factory)
            {
                s_factory->Release();
                s_factory = nullptr;
            }
        }

        void GlobalTerminate()
        {
            LockGuard lock(s_initMutex);
            UnlockedGlobalTerminate();
        }

    private:
        AsyncFileQueue* m_defaultQueue{ nullptr };
    };

    static bool SpinWaitForComplete(IDStorageStatusArray* status, uint32_t index)
    {
        constexpr uint32_t SpinIterations = 10;

        for (uint32_t i = 0; i < SpinIterations; ++i)
        {
            if (status->IsComplete(index))
                return false;

            HE_SPIN_WAIT_PAUSE();
        }

        // Yield the thread if we didn't complete in our spin
        ::SwitchToThread();
        return true;
    }

    static DWORD DStorageCallbackThread(LPVOID)
    {
        // TODO: Because of the queue this always completes callbacks in order, but reads can complete
        // out of order. Need to improve the way we track & dispatch here.
        while (s_callbackRunning)
        {
            CallbackEntry cb;
            s_callbackQueue.wait_dequeue(cb);

            if (!cb.callback)
                continue;

            // Spin wait until the index is marked as available
            while (SpinWaitForComplete(cb.status, cb.index)) {}

            const HRESULT hr = cb.status->GetHResult(cb.index);
            const Result r = FAILED(hr) ? Win32Result(hr) : Result::Success;
            cb.callback(r);
        }

        return 0;
    }

    AsyncFileLoader* _CreateAsyncFileLoader(Allocator& allocator)
    {
        return allocator.New<AsyncFileLoaderImpl>(allocator);
    }
}

#endif
