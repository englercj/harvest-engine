// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/core/compiler.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    /// A lightweight reader/writer lock.
    ///
    /// \note Lock ordering is not guaranteed, recursion is not supported, and multi-process
    /// locking is not supported.
    class RWLock
    {
    public:
        RWLock();
        ~RWLock();

        RWLock(const RWLock&) = delete;
        RWLock(RWLock&&) = delete;
        RWLock& operator=(const RWLock&) = delete;
        RWLock& operator=(RWLock&&) = delete;

        bool TryAcquireRead();
        void AcquireRead();
        void ReleaseRead();

        bool TryAcquireWrite();
        void AcquireWrite();
        void ReleaseWrite();

        bool TryAcquire() { return TryAcquireWrite(); }
        void Acquire() { AcquireWrite(); }
        void Release() { ReleaseWrite(); }

    private:
        friend class ConditionVariable;
        alignas(8) uint8_t m_opaque[HE_PLATFORM_RWLOCK_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// A lightweight mutually exclusive lock.
    ///
    /// \note Lock ordering is not guaranteed, recursion is not supported, and multi-process
    /// locking is not supported.
    class Mutex
    {
    public:
        Mutex();
        ~Mutex();

        Mutex(const Mutex&) = delete;
        Mutex(Mutex&&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        Mutex& operator=(Mutex&&) = delete;

        bool TryAcquire();
        void Acquire();
        void Release();

    private:
        friend class ConditionVariable;
        alignas(8) uint8_t m_opaque[HE_PLATFORM_MUTEX_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// A mutually exclusive lock that supports recursion and multi-process sharing.
    class RecursiveMutex
    {
    public:
        RecursiveMutex();
        ~RecursiveMutex();

        RecursiveMutex(const RecursiveMutex&) = delete;
        RecursiveMutex(RecursiveMutex&&) = delete;
        RecursiveMutex& operator=(const RecursiveMutex&) = delete;
        RecursiveMutex& operator=(RecursiveMutex&&) = delete;

        bool TryAcquire();
        void Acquire();
        void Release();

    private:
        friend class ConditionVariable;
        alignas(8) uint8_t m_opaque[HE_PLATFORM_RECURSIVE_MUTEX_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// RAII wrapper for a mutex that acquires it on construction and releases it upon destruction.
    template <typename T>
    class LockGuard
    {
    public:
        explicit LockGuard(T& mutex) : m_mutex(mutex) { m_mutex.Acquire(); }
        ~LockGuard() { m_mutex.Release(); }

    private:
        friend class ConditionVariable;
        T& m_mutex;
    };

    // --------------------------------------------------------------------------------------------
    /// RAII wrapper for a RWLock that acquires a read lock on construction and releases it upon destruction.
    class ReadLockGuard
    {
    public:
        explicit ReadLockGuard(RWLock& lock) : m_lock(lock) { m_lock.AcquireRead(); }
        ~ReadLockGuard() { m_lock.ReleaseRead(); }

    private:
        friend class ConditionVariable;
        RWLock& m_lock;
    };

    // --------------------------------------------------------------------------------------------
    /// A synchronization primitive that enables threads to wait until a particular condition
    /// occurs.
    ///
    /// \note Condition variables cannot be shared across processes.
    class ConditionVariable
    {
    private:
        /// Helper to grab the mutex in a lock guard generically. Used internally by Wait() so it
        /// can accept any mutex, or lock guard as a parameter.
        /// \internal
        template <typename T> struct MutexHelper{ static T& Get(T& m) { return m; } };
        template <typename T> struct MutexHelper<LockGuard<T>>{ static T& Get(LockGuard<T>& l) { return l.m_mutex; } };

    public:
        ConditionVariable();
        ~ConditionVariable();

        ConditionVariable(const ConditionVariable&) = delete;
        ConditionVariable(ConditionVariable&&) = delete;
        ConditionVariable& operator=(const ConditionVariable&) = delete;
        ConditionVariable& operator=(ConditionVariable&&) = delete;

        void WakeOne();
        void WakeAll();

        template <typename T>
        void Wait(T& mutex)
        {
            WaitMutex(MutexHelper<T>::Get(mutex));
        }

        template <typename T, typename F>
        void Wait(T& mutex, F&& predicate)
        {
            while (!predicate())
            {
                WaitMutex(MutexHelper<T>::Get(mutex));
            }
        }

        template <typename T>
        bool Wait(T& mutex, Duration timeout)
        {
            return WaitMutex(MutexHelper<T>::Get(mutex), timeout);
        }

        template <typename T, typename F>
        bool Wait(T& mutex, F&& predicate, Duration timeout)
        {
            while (!predicate())
            {
                if (!WaitMutex(MutexHelper<T>::Get(mutex), timeout))
                    return predicate();
            }
            return true;
        }

    private:
        // Specializations are defined in the .cpp file for each supported mutex type.
        template <typename T>
        void WaitMutex(T& mutex);

        // Specializations are defined in the .cpp file for each supported mutex type.
        template <typename T>
        bool WaitMutex(T& mutex, Duration timeout);

    private:
        alignas(8) uint8_t m_opaque[HE_PLATFORM_CONDITION_VARIABLE_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// A semaphore synchronization primitive.
    ///
    /// \note Semaphores cannot be shared across processes.
    class Semaphore
    {
    public:
        explicit Semaphore(uint32_t initialCount = 0);
        ~Semaphore();

        Semaphore(const Semaphore&) = delete;
        Semaphore(Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
        Semaphore& operator=(Semaphore&&) = delete;

        void Notify();

        void Wait();
        bool Wait(Duration timeout);


    private:
        alignas(8) uint8_t m_opaque[HE_PLATFORM_SEMAPHORE_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// A synchronization event that can be signaled and waited upon.
    ///
    /// \note SyncEvents cannot be shared across processes.
    class SyncEvent
    {
    public:
        /// Constructs a new synchronization event.
        ///
        /// \param[in] manualReset Optional. When true, the event must be manually reset after
        ///     being waited upon. When false, the event automatically resets itself when a wait
        ///     resolves successfully. Default is true.
        /// \param[in] initiallySignaled Optional. When true, the event is created in a signaled
        ///     state. Default is false.
        explicit SyncEvent(bool manualReset = true, bool initiallySignaled = false);
        ~SyncEvent();

        SyncEvent(const SyncEvent&) = delete;
        SyncEvent(SyncEvent&&) = delete;
        SyncEvent& operator=(const SyncEvent&) = delete;
        SyncEvent& operator=(SyncEvent&&) = delete;

        void Signal();
        void Reset();

        void Wait();
        bool Wait(Duration timeout);

    private:
        alignas(8) uint8_t m_opaque[HE_PLATFORM_SYNC_EVENT_SIZE];
    };
}
