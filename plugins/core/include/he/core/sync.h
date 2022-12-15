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
    /// A lightweight reader/writer lock. Multiple threads can acquire the lock in 'read' mode
    /// while only a single thread can acquire the lock in 'write' mode.
    ///
    /// \note Lock ordering is not guaranteed, recursion is not supported, and multi-process
    /// locking is not supported.
    class RWLock
    {
    public:
        /// Construct a reader-writer lock.
        RWLock() noexcept;

        /// Destruct a reader-writer lock.
        ~RWLock() noexcept;

        RWLock(const RWLock&) = delete;
        RWLock(RWLock&&) = delete;
        RWLock& operator=(const RWLock&) = delete;
        RWLock& operator=(RWLock&&) = delete;

        /// Attempts to acquire the lock in shared read mode, and returns immediately.
        ///
        /// \return Returns true if the lock was acquired, or false if it wasn't.
        bool TryAcquireRead();

        /// Acquires the lock in shared read mode. Blocks until the lock can be acquired.
        void AcquireRead();

        /// Releases the lock that is held in read mode.
        void ReleaseRead();

        /// Attempts to acquire the lock in exclusive write mode, and returns immediately.
        ///
        /// \return Returns true if the lock was acquired, or false if it wasn't.
        bool TryAcquireWrite();

        /// Acquires the lock in exclusive write mode. Blocks until the lock can be acquired.
        void AcquireWrite();

        /// Releases the lock that is held in write mode.
        void ReleaseWrite();

        /// \copydoc TryAcquireWrite
        ///
        /// \note This overload is useful for templates to operate on locks (like \ref LockGuard).
        bool TryAcquire() { return TryAcquireWrite(); }

        /// \copydoc AcquireWrite
        ///
        /// \note This overload is useful for templates to operate on locks (like \ref LockGuard).
        void Acquire() { AcquireWrite(); }

        /// \copydoc ReleaseWrite
        ///
        /// \note This overload is useful for templates to operate on locks (like \ref LockGuard).
        void Release() { ReleaseWrite(); }

    private:
        friend class ConditionVariable;
        alignas(8) uint8_t m_opaque[HE_PLATFORM_RWLOCK_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// A lightweight mutually exclusive lock. Only a single thread can acquire the lock at a time.
    ///
    /// \note Lock ordering is not guaranteed, recursion is not supported, and multi-process
    /// locking is not supported.
    class Mutex
    {
    public:
        /// Construct a mutually exclusive lock.
        Mutex() noexcept;

        /// Destruct a mutually exclusive lock.
        ~Mutex() noexcept;

        Mutex(const Mutex&) = delete;
        Mutex(Mutex&&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        Mutex& operator=(Mutex&&) = delete;

        /// Attempts to acquire the lock, and returns immediately.
        ///
        /// \return Returns true if the lock was acquired, or false if it wasn't.
        bool TryAcquire();

        /// Acquires the lock. Blocks until the lock can be acquired.
        void Acquire();

        /// Releases the lock.
        void Release();

    private:
        friend class ConditionVariable;
        alignas(8) uint8_t m_opaque[HE_PLATFORM_MUTEX_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// A mutually exclusive lock that supports recursion and multi-process sharing.
    /// 
    /// \note Lock ordering is not guaranteed, but recursion and multi-process locking is supported.
    class RecursiveMutex
    {
    public:
        /// Construct a recursive, mutually exclusive, lock.
        RecursiveMutex() noexcept;

        /// Destructa recursive, mutually exclusive, lock.
        ~RecursiveMutex() noexcept;

        RecursiveMutex(const RecursiveMutex&) = delete;
        RecursiveMutex(RecursiveMutex&&) = delete;
        RecursiveMutex& operator=(const RecursiveMutex&) = delete;
        RecursiveMutex& operator=(RecursiveMutex&&) = delete;

        /// Attempts to acquire the lock, and returns immediately.
        ///
        /// \return Returns true if the lock was acquired, or false if it wasn't.
        bool TryAcquire();

        /// Acquires the lock. Blocks until the lock can be acquired.
        void Acquire();

        /// Releases the lock.
        void Release();

    private:
        friend class ConditionVariable;
        alignas(8) uint8_t m_opaque[HE_PLATFORM_RECURSIVE_MUTEX_SIZE];
    };

    // --------------------------------------------------------------------------------------------
    /// RAII wrapper for a lock that acquires it on construction and releases it upon destruction.
    template <typename T>
    class LockGuard
    {
    public:
        /// Construct a lock guard and acquire the lock.
        ///
        /// \param[in] lock The lock to acquire at construction.
        explicit LockGuard(T& lock) noexcept : m_lock(lock) { m_lock.Acquire(); }

        /// Destruct a lock guard and release the lock.
        ~LockGuard() noexcept { m_lock.Release(); }

    private:
        friend class ConditionVariable;
        T& m_lock;
    };

    // --------------------------------------------------------------------------------------------
    /// RAII wrapper for a RWLock that acquires a read lock on construction and releases it upon destruction.
    class ReadLockGuard
    {
    public:
        /// Construct a lock guard and acquire the lock.
        ///
        /// \param[in] lock The lock to acquire at construction.
        explicit ReadLockGuard(RWLock& lock) noexcept : m_lock(lock) { m_lock.AcquireRead(); }

        /// Destruct a lock guard and release the lock.
        ~ReadLockGuard() noexcept { m_lock.ReleaseRead(); }

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
        template <typename T> struct MutexHelper<LockGuard<T>>{ static T& Get(LockGuard<T>& l) { return l.m_lock; } };

    public:
        /// Constructs a condition variable.
        ConditionVariable() noexcept;

        /// Destructs a condition variable.
        ~ConditionVariable() noexcept;

        ConditionVariable(const ConditionVariable&) = delete;
        ConditionVariable(ConditionVariable&&) = delete;
        ConditionVariable& operator=(const ConditionVariable&) = delete;
        ConditionVariable& operator=(ConditionVariable&&) = delete;

        /// Wake a single thread waiting on the condition variable.
        void WakeOne();

        /// Wake all threads waiting on the condition variable.
        void WakeAll();

        /// Waits on the condition variable and releases the specified mutex as an atomic
        /// operation. If the mutex is already unlocked when this function is called, the function
        /// behavior is undefined.
        ///
        /// Once the thread is awoken it re-acquires the mutex it released when Wait() was
        /// originally called.
        ///
        /// \param[in] mutex The mutex to unlock while waiting, and re-acquire after waking.
        template <typename T>
        void Wait(T& mutex)
        {
            WaitMutex(MutexHelper<T>::Get(mutex));
        }

        /// Waits on the condition variable and releases the specified mutex as an atomic
        /// operation. If the mutex is already unlocked when this function is called, the function
        /// behavior is undefined.
        ///
        /// Once the thread is awoken it re-acquires the mutex it released when Wait() was
        /// originally called.
        ///
        /// Since condition variables are subject to spurious wakeups and stolen wakeups the
        /// predicate can be used to check if the wait should conclude or continue. If the
        /// predicate returns `true` the thread will wake, otherwise it will continue to wait.
        ///
        /// \param[in] mutex The mutex to unlock while waiting, and re-acquire after waking.
        /// \param[in] predicate The predicate to check upon wake to know if waiting should continue.
        template <typename T, typename F>
        void Wait(T& mutex, F&& predicate)
        {
            while (!predicate())
            {
                WaitMutex(MutexHelper<T>::Get(mutex));
            }
        }

        /// Waits on the condition variable and releases the specified mutex as an atomic
        /// operation. If the mutex is already unlocked when this function is called, the function
        /// behavior is undefined.
        ///
        /// Once the thread is awoken it re-acquires the mutex it released when Wait() was
        /// originally called.
        ///
        /// \param[in] mutex The mutex to unlock while waiting, and re-acquire after waking.
        /// \param[in] timeout The maximum time to spend waiting for a wake event that passes
        ///     the predicate.
        /// \return Returns true if the thread was woken and the predicate returns true. Otherwise,
        ///     if the wait times out false is returned.
        template <typename T>
        bool Wait(T& mutex, Duration timeout)
        {
            return WaitMutex(MutexHelper<T>::Get(mutex), timeout);
        }

        /// Waits on the condition variable and releases the specified mutex as an atomic
        /// operation. If the mutex is already unlocked when this function is called, the function
        /// behavior is undefined.
        ///
        /// Once the thread is awoken it re-acquires the mutex it released when Wait() was
        /// originally called.
        ///
        /// Since condition variables are subject to spurious wakeups and stolen wakeups the
        /// predicate can be used to check if the wait should conclude or continue. If the
        /// predicate returns `true` the thread will wake, otherwise it will continue to wait.
        ///
        /// \param[in] mutex The mutex to unlock while waiting, and re-acquire after waking.
        /// \param[in] predicate The predicate to check upon wake to know if waiting should continue.
        /// \param[in] timeout The maximum time to spend waiting for a wake event that passes
        ///     the predicate.
        /// \return Returns true if the thread was woken and the predicate returns true. Otherwise,
        ///     if the wait times out false is returned.
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
        /// Construct a semaphore.
        explicit Semaphore(uint32_t initialCount = 0) noexcept;

        /// Destruct a semaphore.
        ~Semaphore() noexcept;

        Semaphore(const Semaphore&) = delete;
        Semaphore(Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
        Semaphore& operator=(Semaphore&&) = delete;

        /// Increases the count of the semaphore by one (1).
        void Notify();

        /// Waits indefinitely for the semaphore to be notified. When it is the thread is awoken
        /// and the count of the semaphore is decreased by one (1).
        void Wait();

        /// Waits indefinitely for the semaphore to be notified. When it is the thread is awoken
        /// and the count of the semaphore is decreased by one (1).
        ///
        /// \param[in] timeout The maximum time to wait for the semaphore to be signaled.
        /// \return Returns true if the semaphore was signaled, and false if it timed out.
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
        /// Constructs a synchronization event.
        ///
        /// \param[in] manualReset Optional. When true, the event must be manually reset after
        ///     being waited upon. When false, the event automatically resets itself when a wait
        ///     resolves successfully. Default is true.
        /// \param[in] initiallySignaled Optional. When true, the event is created in a signaled
        ///     state. Default is false.
        explicit SyncEvent(bool manualReset = true, bool initiallySignaled = false) noexcept;

        /// Destruct a synchronization event.
        ~SyncEvent() noexcept;

        SyncEvent(const SyncEvent&) = delete;
        SyncEvent(SyncEvent&&) = delete;
        SyncEvent& operator=(const SyncEvent&) = delete;
        SyncEvent& operator=(SyncEvent&&) = delete;

        /// Sets the event to the signaled state.
        void Signal();

        /// Sets the event to the nonsignaled state.
        void Reset();

        /// Waits indefinitely for the event to enter the signaled state.
        void Wait();

        /// Waits indefinitely for the event to enter the signaled state.
        ///
        /// \param[in] timeout The maximum time to wait for the event to be signaled.
        /// \return Returns true if the event was signaled, and false if it timed out.
        bool Wait(Duration timeout);

    private:
        alignas(8) uint8_t m_opaque[HE_PLATFORM_SYNC_EVENT_SIZE];
    };
}
