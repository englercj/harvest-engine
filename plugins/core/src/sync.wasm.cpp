// Copyright Chad Engler

#include "he/core/sync.h"

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/thread.h"
#include "he/core/utils.h"

#include <new>

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
        const int32_t tid = BitCast<int32_t>(GetCurrentThreadId());
        int32_t* mutex = reinterpret_cast<int32_t*>(m_opaque);
        int32_t expected = 0;
        return __atomic_compare_exchange_n(mutex, &expected, tid, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
    }

    void Mutex::Acquire()
    {
        const int32_t tid = BitCast<int32_t>(GetCurrentThreadId());
        int32_t* mutex = reinterpret_cast<int32_t*>(m_opaque);
        while (!TryAcquire())
        {
            _AtomicWaitCurrentThread(mutex, tid, -1);
        }
    }

    void Mutex::Release()
    {
        int32_t* mutex = reinterpret_cast<int32_t*>(m_opaque);

    #if HE_ENABLE_ASSERTIONS
        // When assertions are enabled we want to verify that we're releasing a lock that we hold.
        const int32_t value = __atomic_exchange_n(mutex, 0, __ATOMIC_RELEASE);
        HE_ASSERT(value == GetCurrentThreadId(), HE_MSG("Mutex::Release() called on a lock that was not held by this thread"));
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

        const uint32_t tid = GetCurrentThreadId();

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
    enum _CVWaiterState : int32_t
    {
        _CVWaiterState_Waiting,
        _CVWaiterState_Signaled,
        _CVWaiterState_Leaving,
    };

    struct _CVWaiter
    {
        _CVWaiter* prev{ nullptr };
        _CVWaiter* next{ nullptr };
        volatile int32_t* notify{ nullptr };
        volatile int32_t state{ _CVWaiterState_Waiting };
        volatile int32_t barrier{ 0 };
    };

    struct _CVData
    {
        _CVData* head{ nullptr };
        _CVData* tail{ nullptr };
        volatile int32_t lock{ 0 };
    };

    static void _ConditionVariable_LockInt(volatile int* lock)
    {
        int32_t expected = 0;
        if (!__atomic_compare_exchange_n(lock, &expected, 1, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        {
            expected = 1;
            __atomic_compare_exchange_n(lock, &expected, 2, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

            expected = 0;
            do
            {
                _AtomicWaitCurrentThread(lock, 2, -1);
            }
            while (!__atomic_compare_exchange_n(lock, &expected, 2, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
        }
    }

    static void _ConditionVariable_UnlockInt(volatile int* lock)
    {
        if (__atomic_exchange_n(lock, 0, __ATOMIC_SEQ_CST) == 2)
        {
            __builtin_wasm_memory_atomic_notify(lock, 1);
        }
    }

    static void _ConditionVariable_UnlockIntRequeue(volatile int* lock)
    {
        __atomic_store_n(lock, 0, __ATOMIC_SEQ_CST);
        // Here the intent is to wake one waiter, and requeue all other waiters from waiting on
        // address 'lock' to wait on a new lock address instead. This is not possible at the
        // moment with SharedArrayBuffer Atomics, as it does not have a "wake X waiters and
        // requeue the rest" primitive. However this kind of primitive is strictly not needed,
        // since it is more like an optimization to avoid spuriously waking all waiters, just
        // to make them wait on another location immediately afterwards. Here we do exactly that:
        // wake every waiter.
        __builtin_wasm_memory_atomic_notify(lock, 0xffffffff);
    }

    static void _ConditionVariable_Signal(_CVData* data, uint32_t count)
    {
        _CVWaiter* p = nullptr;
        _CVWaiter* first = nullptr;
        volatile int32_t ref = 0;

        _ConditionVariable_LockInt(&data->lock);

        // Mark `count` waiters as signaled.
        for (_CVWaiter* p = data->tail; count && p; p = p->prev)
        {
            int32_t expected = _CVWaiterState_Waiting;
            if (!__atomic_compare_exchange_n(&p->state, &expected, _CVWaiterState_Signaled, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
            {
                ++ref;
                p->notify = &ref;
            }
            else
            {
                --count;
                if (!first)
                {
                    first = p;
                }
            }
        }

        // Split the list, leaving any remainder on the cv.
        if (p)
        {
            if (p->next)
                p->next->prev = nullptr;
            p->next = nullptr;
        }
        else
        {
            data->head = nullptr;
        }
        data->tail = p;

        _ConditionVariable_UnlockInt(&data->lock);

        // Wait for any waiters in the leaving state to remove themselves from the list before
        // allowing signaled threads to proceed.
        while (int32_t cur = __atomic_load_n(&ref))
        {
            _AtomicWaitCurrentThread(&ref, cur, -1);
        }

        // Allow first signaled waiter, if any, to proceed.
        if (first)
        {
            _ConditionVariable_UnlockInt(&first->barrier);
        }
    }

    template <typename T>
    void _ConditionVariable_WaitMutex(_CVData* data, T& mutex, Duration timeout)
    {
        // Setup the waiter list by creating a new node and making it the head.
        _ConditionVariable_LockInt(&data->lock);

        _CVWaiter node{};
        node.barrier = 2;
        node.state = _CVWaiterState_Waiting;
        node.next = data->head;

        data->head = &node;

        if (!data->tail)
            data->tail = &node;
        else
            node.next->prev = &node;

        int32_t seq = node.barrier;
        volatile int32_t* fut = &node.barrier;

        _ConditionVariable_UnlockInt(&data->lock);

        // Release the mutex and wait for a signal.
        mutex.Release();

        YieldCurrentThread(); // Maybe not needed?

        const MonotonicTime waitEnd = MonotonicClock::Now() + timeout;
        bool timedOut = false;
        do
        {
            // Calculate how much wait time is left, and if it has expired we've timed out.
            const Duration wait = end - MonotonicClock::Now();
            if (wait <= Duration_Zero)
            {
                timedOut = true;
                break;
            }

            // Wait for a signal or timeout.
            const int32_t rc = _AtomicWaitCurrentThread(fut, seq, wait.val);

            // the timeout expired during the wait
            if (rc == 2)
            {
                timedOut = true;
                break;
            }
        } while (__atomic_load_n(fut, __ATOMIC_SEQ_CST) == seq);

        _CVWaiterState oldState = _CVWaiterState_Waiting;
        __atomic_compare_exchange_n(&node.state, &oldState, _CVWaiterState_Leaving, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

        if (oldState == _CVWaiterState_Waiting)
        {
            // Remove the node from the list.
            _ConditionVariable_LockInt(&data->lock);

            if (data->head == &node)
                data->head = node.next;
            else if (node.prev)
                node.prev->next = node.next;

            if (data->tail == &node)
                data->tail = node.prev;
            else if (node.next)
                node.next->prev = node.prev;

            _ConditionVariable_UnlockInt(&data->lock);

            // If the node was not signaled, wake the next waiter.
            if (node.notify)
            {
                if (__atomic_fetch_sub(node.notify, 1, __ATOMIC_SEQ_CST) == 1)
                {
                    __builtin_wasm_memory_atomic_notify(node.notify, 1);
                }
            }
        }
        else
        {
            // Lock barrier first to control wake order.
            _ConditionVariable_LockInt(&node.barrier);
        }

        mutex.Acquire();

        // Unlock the barrier that's holding back the next waiters. This make them fall into the
        // mutex.Acquire() above effectively requeueing them to wait on the mutex.
        if (oldState != _CVWaiterState_Waiting && node.prev)
        {
            _ConditionVariable_UnlockIntRequeue(&node.prev->barrier);
        }
    }

    ConditionVariable::ConditionVariable() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(_CVData));
        HE_ASSERT(IsAligned(m_opaque, alignof(_CVData)));

        ::new(m_opaque) _CVData();
    }

    ConditionVariable::~ConditionVariable() noexcept
    {
        _CVData* data = reinterpret_cast<_CVData*>(m_opaque);
        data->~_CVData();
    }

    void ConditionVariable::WakeOne()
    {
        _CVData* data = reinterpret_cast<_CVData*>(m_opaque);
        return _ConditionVariable_Signal(data, 1);
    }

    void ConditionVariable::WakeAll()
    {
        _CVData* data = reinterpret_cast<_CVData*>(m_opaque);
        return _ConditionVariable_Signal(data, 0xffffffff);
    }

    template <>
    void ConditionVariable::WaitMutex<Mutex>(Mutex& mutex)
    {
        WaitMutex(mutex, Duration_Max);
    }

    template <>
    void ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex)
    {
        WaitMutex(mutex, Duration_Max);
    }

    template <>
    bool ConditionVariable::WaitMutex<Mutex>(Mutex& mutex, Duration timeout)
    {
        // Trying to await a condition variable on a thread that cannot block is not yet supported.
        // I'm skeptical this feature is even needed, but if it is it will require some work to
        // implement a special scheme that doesn't block but still waits.
        HE_ASSERT(_CanCurentThreadWait());

        _CVData* data = reinterpret_cast<_CVData*>(m_opaque);
        _ConditionVariable_WaitMutex(data, mutex, timeout);
    }

    template <>
    bool ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex, Duration timeout)
    {
        // Trying to await a condition variable on a thread that cannot block is not yet supported.
        // I'm skeptical this feature is even needed, but if it is it will require some work to
        // implement a special scheme that doesn't block but still waits.
        HE_ASSERT(_CanCurentThreadWait());

        _CVData* data = reinterpret_cast<_CVData*>(m_opaque);
        _ConditionVariable_WaitMutex(data, mutex, timeout);
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

    static bool _Semaphore_TryAcquire(uint32_t* sem)
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

        while (!_Semaphore_TryAcquire(sem))
        {
            _AtomicWaitCurrentThread(sem, 0, -1);
        }
    }

    bool Semaphore::Wait(Duration timeout)
    {
        uint32_t* sem = reinterpret_cast<uint32_t*>(m_opaque);

        if (timeout == Duration_Zero)
        {
            return _Semaphore_TryAcquire(sem);
        }

        const MonotonicTime end = MonotonicClock::Now() + timeout;
        while (true)
        {
            if (_Semaphore_TryAcquire(sem))
                return true;

            // Calculate how much wait time is left, and if it has expired we've timed out.
            const Duration wait = end - MonotonicClock::Now();
            if (wait <= Duration_Zero)
                return false;

            const int32_t rc = _AtomicWaitCurrentThread(sem, 0, wait.val);

            // the timeout expired during the wait
            if (rc == 2)
                return false;
        }

        return false;
    }

    // --------------------------------------------------------------------------------------------
    struct _SyncEventData
    {
        Mutex mutex{};
        ConditionVariable cv{};
        bool state{ false };
        bool manualReset{ false };
    };

    SyncEvent::SyncEvent(bool manualReset, bool initiallySignaled) noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(_SyncEventData));
        HE_ASSERT(IsAligned(m_opaque, alignof(_SyncEventData)));

        _SyncEventData* data = ::new(m_opaque) _SyncEventData();
        data->manualReset = manualReset;
        data->state = initiallySignaled;
    }

    SyncEvent::~SyncEvent() noexcept
    {
        _SyncEventData* data = reinterpret_cast<_SyncEventData*>(m_opaque);
        data->~_SyncEventData();
    }

    void SyncEvent::Signal()
    {
        _SyncEventData* data = reinterpret_cast<_SyncEventData*>(m_opaque);

        LockGuard lock(data->mutex);

        data->state = true;
        data->cv.WakeAll();
    }

    void SyncEvent::Reset()
    {
        _SyncEventData* data = reinterpret_cast<_SyncEventData*>(m_opaque);

        LockGuard lock(data->mutex);

        data->state = false;
    }

    void SyncEvent::Wait()
    {
        LockGuard lock(data->mutex);

        _SyncEventData* data = reinterpret_cast<_SyncEventData*>(m_opaque);
        data->cv.Wait(data->mutex, [data]() { return data->state; });
        if (!data->manualReset)
            data->state = false;
    }

    bool SyncEvent::Wait(Duration timeout)
    {
        _SyncEventData* data = reinterpret_cast<_SyncEventData*>(m_opaque);

        LockGuard lock(data->mutex);

        const bool result = data->cv.Wait(data->mutex, [data]() { return data->state; }, timeout);
        if (result && !data->manualReset)
            data->state = false;

        return result;
    }
}

#endif
