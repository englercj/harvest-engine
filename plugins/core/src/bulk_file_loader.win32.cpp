// Copyright Chad Engler

// TODO: Implement compression handling using `IDStorageCustomDecompressionQueue`

#include "he/core/bulk_file_loader.h"

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/result.h"
#include "he/core/scope_guard.h"
#include "he/core/sync.h"
#include "he/core/thread.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"

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

    class RequestTracker
    {
    public:
        RequestTracker()
        {
            m_event = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
            HE_ASSERT(m_event);

            auto callback = [](TP_CALLBACK_INSTANCE*, void* context, TP_WAIT*, TP_WAIT_RESULT)
            {
                RequestTracker* tracker = reinterpret_cast<RequestTracker*>(context);
                if (tracker->m_callback)
                    tracker->m_callback(*tracker);
            };

            m_wait = ::CreateThreadpoolWait(callback, this, nullptr);
            HE_ASSERT(m_wait);
        }

        ~RequestTracker()
        {
            if (m_wait)
            {
                ::WaitForThreadpoolWaitCallbacks(m_wait, TRUE);
                ::CloseThreadpoolWait(m_wait);
            }

            if (m_event)
            {
                ::CloseHandle(m_event);
            }
        }

        void Reset(BulkFileQueue::LoadDelegate callback, uint32_t index)
        {
            m_callback = callback;
            m_index = index;

            ::ResetEvent(m_event);

            if (m_callback)
                ::SetThreadpoolWait(m_wait, m_event, nullptr);
            else
                ::SetThreadpoolWait(m_wait, nullptr, nullptr);
        }

        bool IsSet() const
        {
            return ::WaitForSingleObject(m_event, 0) == WAIT_OBJECT_0;
        }

        void Wait() const { ::WaitForSingleObject(m_event, INFINITE); }
        HANDLE Event() const { return m_event; }
        uint32_t Index() const { return m_index; }

        operator BulkReadId() const { return BulkReadId{ reinterpret_cast<uintptr_t>(this) }; }

    private:
        BulkFileQueue::LoadDelegate m_callback{};
        uint32_t m_index{ 0 };

        HANDLE m_event{ nullptr };
        TP_WAIT* m_wait{ nullptr };
    };

    DSTORAGE_COMPRESSION_FORMAT ToDsCompressionFormat(BulkReadRequest::CompressionFormat x)
    {
        switch (x)
        {
            case BulkReadRequest::CompressionFormat::None: return DSTORAGE_COMPRESSION_FORMAT_NONE;
            case BulkReadRequest::CompressionFormat::Zlib: return DSTORAGE_CUSTOM_COMPRESSION_0;
            case BulkReadRequest::CompressionFormat::GDeflate: return DSTORAGE_COMPRESSION_FORMAT_GDEFLATE;
        }

        HE_VERIFY(false, HE_MSG("Unknown compression format"), HE_KV(format, x));
        return DSTORAGE_COMPRESSION_FORMAT_NONE;
    }

    DSTORAGE_PRIORITY ToDsPriority(BulkFileQueue::Config::Priority x)
    {
        switch (x)
        {
            case BulkFileQueue::Config::Priority::Low: return DSTORAGE_PRIORITY_LOW;
            case BulkFileQueue::Config::Priority::Normal: return DSTORAGE_PRIORITY_NORMAL;
            case BulkFileQueue::Config::Priority::High: return DSTORAGE_PRIORITY_HIGH;
            case BulkFileQueue::Config::Priority::Realtime: return DSTORAGE_PRIORITY_REALTIME;
        }

        HE_VERIFY(false, HE_MSG("Unknown queue priority"), HE_KV(priority, x));
        return DSTORAGE_PRIORITY_NORMAL;
    }

    class BulkFileQueueImpl final : public BulkFileQueue
    {
    public:
        BulkFileQueueImpl(Allocator& allocator) : m_allocator(allocator) {}

        ~BulkFileQueueImpl()
        {
            if (m_status)
            {
                m_status->Release();
                m_status = nullptr;
            }

            if (m_queue)
            {
                m_queue->CancelRequestsWithTag(~0ull, reinterpret_cast<uint64_t>(this));
                m_queue->Release();
                m_queue = nullptr;
            }
        }

        Result Initialize(const Config& config)
        {
            const char* name = config.name;

            m_capacity = config.capacity;

            if (!HE_VERIFY(m_capacity >= DSTORAGE_MIN_QUEUE_CAPACITY && m_capacity <= DSTORAGE_MAX_QUEUE_CAPACITY,
                HE_MSG("Queue capacity must be within DirectStorage limits. The value will be clamped"),
                HE_KV(capacity, m_capacity),
                HE_KV(min_capacity, DSTORAGE_MIN_QUEUE_CAPACITY),
                HE_KV(max_capacity, DSTORAGE_MAX_QUEUE_CAPACITY)))
            {
                m_capacity = Clamp<uint16_t>(m_capacity, DSTORAGE_MIN_QUEUE_CAPACITY, DSTORAGE_MAX_QUEUE_CAPACITY);
            }

            DSTORAGE_QUEUE_DESC desc{};
            desc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            desc.Capacity = m_capacity;
            desc.Priority = ToDsPriority(config.priority);
            desc.Name = name;
            desc.Device = nullptr;

            HRESULT hr = s_factory->CreateQueue(&desc, IID_PPV_ARGS(&m_queue));
            if (FAILED(hr))
                return Win32Result(hr);

            hr = s_factory->CreateStatusArray(m_capacity, name, IID_PPV_ARGS(&m_status));
            if (FAILED(hr))
                return Win32Result(hr);

            return Result::Success;
        }

        void Enqueue(const BulkReadRequest& req) override
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
            dsReq.CancellationTag = reinterpret_cast<uint64_t>(this);
            dsReq.Name = req.name;

            m_queue->EnqueueRequest(&dsReq);
        }

        BulkReadId Submit(LoadDelegate callback) override
        {
            const uint32_t index = static_cast<uint32_t>(m_index++ % m_capacity);
            // TODO: Pool these allocations in an object pool
            RequestTracker* tracker = m_allocator.New<RequestTracker>();

            tracker->Reset(callback, index);
            m_queue->EnqueueStatus(m_status, tracker->Index());
            m_queue->EnqueueSetEvent(tracker->Event());
            m_queue->Submit();

            return *tracker;
        }

        bool IsComplete(BulkReadId token) const override
        {
            RequestTracker* tracker = reinterpret_cast<RequestTracker*>(token.val);
            return tracker->IsSet();
        }

        Result GetResult(BulkReadId token) const override
        {
            RequestTracker* tracker = reinterpret_cast<RequestTracker*>(token.val);
            tracker->Wait();

            const HRESULT hr = m_status->GetHResult(tracker->Index());
            HE_ASSERT(hr != E_PENDING);

            m_allocator.Delete(tracker);
            return FAILED(hr) ? Win32Result(hr) : Result::Success;
        }

    private:
        Allocator& m_allocator;

        IDStorageQueue1* m_queue{ nullptr };
        IDStorageStatusArray* m_status{ nullptr };
        std::atomic<uint64_t> m_index{ 0 };

        uint16_t m_capacity{ 0 };
    };

    class BulkFileLoaderImpl final : public BulkFileLoader
    {
    public:
        BulkFileLoaderImpl(Allocator& allocator) : BulkFileLoader(allocator) {}

        ~BulkFileLoaderImpl()
        {
            GlobalTerminate();
        }

        Result Initialize(const Config& config) override
        {
            Result r = GlobalInitialize(config);
            if (!r)
                return r;

            return Result::Success;
        }

        Result OpenFile(const char* path, BulkFileId& fd) override
        {
            IDStorageFile* file = nullptr;
            HRESULT hr = s_factory->OpenFile(HE_TO_WCSTR(path), IID_PPV_ARGS(&file));
            fd.val = reinterpret_cast<uintptr_t>(file);
            return FAILED(hr) ? Win32Result(hr) : Result::Success;
        }

        void CloseFile(BulkFileId fd) override
        {
            IDStorageFile* file = reinterpret_cast<IDStorageFile*>(fd.val);
            if (file)
                file->Release();
        }

        Result GetAttributes(BulkFileId fd, FileAttributes& outAttributes)
        {
            IDStorageFile* file = reinterpret_cast<IDStorageFile*>(fd.val);

            BY_HANDLE_FILE_INFORMATION info{};
            HRESULT hr = file->GetFileInformation(&info);

            if (FAILED(hr))
                return Win32Result(hr);

            Win32ParseFileAttributes(info, outAttributes);
            return Result::Success;
        }

        Result CreateQueue(const BulkFileQueue::Config& config, BulkFileQueue*& out) override
        {
            BulkFileQueueImpl* queue = m_allocator.New<BulkFileQueueImpl>();
            Result r = queue->Initialize(config);
            if (!r)
            {
                m_allocator.Delete(queue);
                queue = nullptr;
            }

            out = queue;
            return r;
        }

        void DestroyQueue(BulkFileQueue* queue) override
        {
            m_allocator.Delete(queue);
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
            dsConfig.NumSubmitThreads = config.dstorage.submitThreadCount;
            dsConfig.NumBuiltInCpuDecompressionThreads = config.dstorage.decompressThreadCount;
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

            failGuard.Dismiss();
            return Result::Success;
        }

        void UnlockedGlobalTerminate()
        {
            HE_ASSERT(s_initCount > 0);
            const uint32_t count = --s_initCount;
            if (count > 0)
                return;

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
    };

    BulkFileLoader* _CreateBulkFileLoader(Allocator& allocator)
    {
        return allocator.New<BulkFileLoaderImpl>(allocator);
    }
}

#endif
