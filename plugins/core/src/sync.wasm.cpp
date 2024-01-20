// Copyright Chad Engler

#include "he/core/sync.h"

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/thread.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_WASM)

#include "thread_internal.wasm.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    RWLock::RWLock() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(int32_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(int32_t)));

        int32_t* rwlock = reinterpret_cast<int32_t*>(m_opaque);
        *rwlock = 0;
    }

    RWLock::~RWLock() noexcept
    {
        HE_ASSERT(TryAcquireWrite() && (ReleaseWrite(), true));
    }

    bool RWLock::TryAcquireRead()
    {
        int32_t* rwlock = reinterpret_cast<int32_t*>(m_opaque);
        int32_t expected = 0;

        while (!__atomic_compare_exchange_n(&mutex, &expected, expected + 1, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        {
            if (expected == -1)
                return false;
        }
        return true;
    }

    void RWLock::AcquireRead()
    {
        int32_t* rwlock = reinterpret_cast<int32_t*>(m_opaque);

        while (!TryAcquireRead())
        {
            // Wait if the lock is held by a writer.
            _AtomicWaitCurrentThread(rwlock, -1, -1);
        }
    }

    void RWLock::ReleaseRead()
    {
        int32_t* rwlock = reinterpret_cast<int32_t*>(m_opaque);
        const int32_t value = __atomic_fetch_sub(rwlock, 1, __ATOMIC_RELEASE);
        HE_UNUSED(value);
        HE_ASSERT(value >= 0, HE_MSG("RWLock::ReleaseRead() called on a lock that has a write lock held"));

        // Notify only a single agent. Readers won't be waiting on this release, and only one
        // writer will be able to take the lock anyway.
        __builtin_wasm_memory_atomic_notify(rwlock, 1);
    }

    bool RWLock::TryAcquireWrite()
    {
        int32_t* rwlock = reinterpret_cast<int32_t*>(m_opaque);
        int32_t expected = 0;
        return __atomic_compare_exchange_n(rwlock, &expected, -1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
    }

    void RWLock::AcquireWrite()
    {
        int32_t* rwlock = reinterpret_cast<int32_t*>(m_opaque);
        int32_t expected = 0;

        // This is a strong cas, despite the loop, because if we fail spuriously we might end up
        // waiting when the lock value is zero and missing our opportunity to acquire it.
        while (!__atomic_compare_exchange_n(rwlock, &expected, -1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        {
            // Wait if the lock is held by anyone. Our wait here races with someone else
            // modifying the lock value, but in that case the wait will return immediately and
            // we'll try to acquire again.
            _AtomicWaitCurrentThread(rwlock, expected, -1);
        }
    }

    void RWLock::ReleaseWrite()
    {
        int32_t* rwlock = reinterpret_cast<int32_t*>(m_opaque);
    #if HE_ENABLE_ASSERTIONS
        // When assertions are enabled we want to verify that unlocking was legit so do an exchange.
        const int32_t value = __atomic_exchange_n(rwlock, 0, __ATOMIC_RELEASE);
        HE_ASSERT(value == -1, HE_MSG("RWLock::ReleaseWrite() called on a lock that has a read lock held"));
    #else
        // When assertions are disabled we don't care about the existing value, so just do a store.
        __atomic_store_n(rwlock, 0, __ATOMIC_RELEASE);
    #endif

        // Notify all agents. Any number of readers or writers may be waiting on this release.
        __builtin_wasm_memory_atomic_notify(rwlock, 0xffffffff);
    }

    // --------------------------------------------------------------------------------------------
    Mutex::Mutex() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(int32_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(int32_t)));

        int32_t* mutex = reinterpret_cast<int32_t*>(m_opaque);
        *mutex = 0;
    }

    Mutex::~Mutex() noexcept
    {
        HE_ASSERT(TryAcquire() && (Release(), true));
    }

    bool Mutex::TryAcquire()
    {
        int32_t* mutex = reinterpret_cast<int32_t*>(m_opaque);
        int32_t expected = 0;
        return __atomic_compare_exchange_n(mutex, &expected, 1, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }

    void Mutex::Acquire()
    {
        int32_t* mutex = reinterpret_cast<int32_t*>(m_opaque);
        while (!TryAcquire())
        {
            _AtomicWaitCurrentThread(mutex, 1, -1);
        }
    }

    void Mutex::Release()
    {
        int32_t* mutex = reinterpret_cast<int32_t*>(m_opaque);

    #if HE_ENABLE_ASSERTIONS
        // When assertions are enabled we want to verify that unlocking was legit so do an exchange.
        const int32_t value = __atomic_exchange_n(mutex, 0, __ATOMIC_RELEASE);
        HE_ASSERT(value == 1, HE_MSG("Mutex::Release() called on a lock that was not held"));
    #else
        // When assertions are disabled we don't care about the existing value, so just do a store.
        __atomic_store_n(mutex, 0, __ATOMIC_RELEASE);
    #endif

        // Notify one agent. Since the lock is exclusive only one of them can acquire it anyway.
        __builtin_wasm_memory_atomic_notify(mutex, 1);
    }

    // --------------------------------------------------------------------------------------------
    RecursiveMutex::RecursiveMutex() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(int32_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(int32_t)));

        static_assert(sizeof(m_opaque + 4) == sizeof(int32_t));
        HE_ASSERT(IsAligned(m_opaque + 4, alignof(int32_t)));

        static_assert(sizeof(m_opaque) == sizeof(uint64_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(uint64_t)));

        uint64_t* mutex = reinterpret_cast<uint64_t*>(m_opaque);
        *mutex = 0;
    }

    RecursiveMutex::~RecursiveMutex() noexcept
    {
    #if HE_ENABLE_ASSERTIONS
        uint64_t* mutex = reinterpret_cast<uint64_t*>(m_opaque);
        HE_ASSERT(__atomic_load_n(mutex, __ATOMIC_SEQ_CST) == 0);
    #endif
    }

    static uint32_t _RecursiveMutex_TryAcquire(uint8_t* opaque)
    {
        static constexpr uint64_t TidMask = 0x00000000ffffffffull;
        static constexpr uint64_t CountIncrement = 0x0000000100000000ull;

        static thread_local const uint32_t tid = GetCurrentThreadId();

        uint64_t* mutex = reinterpret_cast<uint64_t*>(m_opaque);
        uint64_t expected = 0;
        uint64_t desired = CountIncrement | tid;

        while (!__atomic_compare_exchange_n(&mutex, &expected, desired, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        {
            const uint32_t lockedTid = static_cast<uint32_t>(expected & TidMask);
            if (lockedTid != tid)
                return lockedTid;

            desired = expected + CountIncrement;
        }

        return 0;
    }

    bool RecursiveMutex::TryAcquire()
    {
        return _RecursiveMutex_TryAcquire(m_opaque) == 0;
    }

    void RecursiveMutex::Acquire()
    {
        int32_t* threadId = reinterpret_cast<int32_t*>(m_opaque + 4);
        uint32_t lockedTid = 0;

        while ((lockedTid = _RecursiveMutex_TryAcquire(m_opaque)) != 0)
        {
            _AtomicWaitCurrentThread(threadId, static_cast<int32_t>(lockedTid), -1);
        }
    }

    void RecursiveMutex::Release()
    {
        uint64_t* mutex = reinterpret_cast<uint64_t*>(m_opaque);
    #if HE_ENABLE_ASSERTIONS
        // When assertions are enabled we want to verify that unlocking was legit so do an exchange.
        const uint64_t value = __atomic_exchange_n(mutex, 0, __ATOMIC_RELEASE);
        HE_ASSERT((value & 0xffffffff) == GetCurrentThreadId(), HE_MSG("RecursiveMutex::Release() called on a lock that is held by another thread."));
    #else
        // When assertions are disabled we don't care about the existing value, so just do a store.
        __atomic_store_n(mutex, 0, __ATOMIC_RELEASE);
    #endif

        // Notify one agent. Since the lock is exclusive only one of them can acquire it anyway.
        __builtin_wasm_memory_atomic_notify(mutex, 1);
    }

    // --------------------------------------------------------------------------------------------
    // TODO: CV still needs to be written.
    ConditionVariable::ConditionVariable() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(uint32_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(uint32_t)));

        uint32_t* cv = reinterpret_cast<uint32_t*>(m_opaque);
        *cv = 0;
    }

    ConditionVariable::~ConditionVariable() noexcept
    {
    }

    void ConditionVariable::WakeOne()
    {
        uint32_t* cv = reinterpret_cast<uint32_t*>(m_opaque);
        // __atomic_fetch_sub(sem, 1, __ATOMIC_RELEASE);
        __builtin_wasm_memory_atomic_notify(cv, 1);
    }

    void ConditionVariable::WakeAll()
    {
        uint32_t* cv = reinterpret_cast<uint32_t*>(m_opaque);
        // __atomic_store_n(sem, 0, __ATOMIC_RELEASE);
        __builtin_wasm_memory_atomic_notify(cv, 0xffffffff);
    }

    template <>
    void ConditionVariable::WaitMutex<Mutex>(Mutex& mutex)
    {
        mutex.Release();

        uint32_t* cv = reinterpret_cast<uint32_t*>(m_opaque);
        _AtomicWaitCurrentThread(cv, 0, -1);

        mutex.Acquire();
    }

    template <>
    void ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex)
    {
        mutex.Release();

        uint32_t* cv = reinterpret_cast<uint32_t*>(m_opaque);
        _AtomicWaitCurrentThread(cv, 0, -1);

        mutex.Acquire();
    }

    template <>
    bool ConditionVariable::WaitMutex<Mutex>(Mutex& mutex, Duration timeout)
    {
    }

    template <>
    bool ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex, Duration timeout)
    {
    }

    // --------------------------------------------------------------------------------------------
    Semaphore::Semaphore(uint32_t initialCount) noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(uint32_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(uint32_t)));

        uint32_t* sem = reinterpret_cast<uint32_t*>(m_opaque);
        *sem = 0;
    }

    Semaphore::~Semaphore() noexcept
    {
    }

    void Semaphore::Notify(uint32_t count)
    {
        uint32_t* sem = reinterpret_cast<uint32_t*>(m_opaque);
        __atomic_fetch_add(sem, count, __ATOMIC_RELEASE);
        __builtin_wasm_memory_atomic_notify(sem, count);
    }

    static bool Semaphore_TryAcquire(uint32_t* sem)
    {
        uint32_t expected = __atomic_load_n(sem, __ATOMIC_ACQUIRE);
        while (expected > 0)
        {
            if (__atomic_compare_exchange_n(sem, &expected, expected - 1, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return true;
        }
        return false;
    }

    void Semaphore::Wait()
    {
        uint32_t* sem = reinterpret_cast<uint32_t*>(m_opaque);

        while (!Semaphore_TryAcquire(sem))
        {
            _AtomicWaitCurrentThread(sem, 0, -1);
        }
    }

    bool Semaphore::Wait(Duration timeout)
    {
        uint32_t* sem = reinterpret_cast<uint32_t*>(m_opaque);

        if (timeout == Duration_Zero)
        {
            return Semaphore_TryAcquire(sem);
        }

        while (timeout.val > 0)
        {
            if (Semaphore_TryAcquire(sem))
                return true;

            const MonotonicTime start = MonotonicClock::now();
            const int32_t rc = _AtomicWaitCurrentThread(sem, 0, timeout.val);
            const MonotonicTime end = MonotonicClock::now();

            // the timeout expired during the wait
            if (rc == 2)
                return false;

            // Normal wait completed, subtract the time waited from the timeout.
            if (rc == 0)
            {
                // Never let timeout go to zero here because we should always try one more time
                // when the wait doesn't give us a timeout return value.
                const int64_t elapsed = (end - start).val;
                const int64_t maxDiff = timeout.val - 1;
                timeout.val -= Min(elapsed, maxDiff);
                HE_ASSERT(timeout.val > 0);
            }
        }

        return false;
    }

    // --------------------------------------------------------------------------------------------
    struct SyncEventData
    {
        Mutex mutex{};
        ConditionVariable cv{};
        bool state{ false };
        bool manualReset{ false };
    };

    SyncEvent::SyncEvent(bool manualReset, bool initiallySignaled) noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(SyncEventData));
        HE_ASSERT(IsAligned(m_opaque, alignof(SyncEventData)));

        SyncEventData* data = new(m_opaque) SyncEventData();
        data->manualReset = manualReset;
        data->state = initiallySignaled;
    }

    SyncEvent::~SyncEvent() noexcept
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);
        data->~SyncEventData();
    }

    void SyncEvent::Signal()
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);

        LockGuard lock(data->mutex);

        data->state = true;
        data->cv.WakeAll();
    }

    void SyncEvent::Reset()
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);

        LockGuard lock(data->mutex);

        data->state = false;
    }

    void SyncEvent::Wait()
    {
        LockGuard lock(data->mutex);

        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);
        data->cv.Wait(data->mutex, [data]() { return data->state; });
        if (!data->manualReset)
            data->state = false;
    }

    bool SyncEvent::Wait(Duration timeout)
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);

        LockGuard lock(data->mutex);

        const bool result = data->cv.Wait(data->mutex, [data]() { return data->state; }, timeout);
        if (result && !data->manualReset)
            data->state = false;

        return result;
    }
}

#endif
