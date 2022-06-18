// Copyright Chad Engler

#include "he/core/sync.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/memory_ops.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX)

#if defined(__linux__)
    #include <sys/eventfd.h>
    #include <sys/epoll.h>
#endif

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#if HE_ENABLE_ASSERTIONS
    #define HE_ASSERT_PTHREAD(expr) \
        do { \
            const int r_ = (expr); \
            HE_ASSERT(r_ == 0, HE_KV(check_expr, #expr), HE_KV(result, PosixResult(r_))); \
        } while (0)

    #define HE_ASSERT_ERRNO(expr) \
        do { \
            const int r_ = (expr); \
            HE_ASSERT(r_ == 0, HE_KV(check_expr, #expr), HE_KV(result, Result::FromLastError())); \
        } while (0)
#else
    #define HE_ASSERT_PTHREAD(expr) (expr)
    #define HE_ASSERT_ERRNO(expr) (expr)
#endif

namespace he
{
    // --------------------------------------------------------------------------------------------
    RWLock::RWLock() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(pthread_rwlock_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(pthread_rwlock_t)));

        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_rwlock_init(rwlock, nullptr));
    }

    RWLock::~RWLock() noexcept
    {
        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_rwlock_destroy(rwlock));
    }

    bool RWLock::TryAcquireRead()
    {
        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        const int r = pthread_rwlock_tryrdlock(rwlock);
        HE_ASSERT(r == 0 || r == EBUSY, HE_KV(check_expr, "pthread_rwlock_tryrdlock(rwlock)"), HE_KV(result, PosixResult(r)));
        return r == 0;
    }

    void RWLock::AcquireRead()
    {
        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_rwlock_rdlock(rwlock));
    }

    void RWLock::ReleaseRead()
    {
        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_rwlock_unlock(rwlock));
    }

    bool RWLock::TryAcquireWrite()
    {
        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        const int r = pthread_rwlock_trywrlock(rwlock);
        HE_ASSERT(r == 0 || r == EBUSY, HE_KV(check_expr, "pthread_rwlock_trywrlock(rwlock)"), HE_KV(result, PosixResult(r)));
        return r == 0;
    }

    void RWLock::AcquireWrite()
    {
        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_rwlock_wrlock(rwlock));
    }

    void RWLock::ReleaseWrite()
    {
        pthread_rwlock_t* rwlock = reinterpret_cast<pthread_rwlock_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_rwlock_unlock(rwlock));
    }

    // --------------------------------------------------------------------------------------------
    Mutex::Mutex() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(pthread_mutex_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(pthread_mutex_t)));

        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);

    #if HE_ENABLE_ASSERTIONS
        pthread_mutexattr_t attr;
        HE_ASSERT_PTHREAD(pthread_mutexattr_init(&attr));
        HE_ASSERT_PTHREAD(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));

        HE_ASSERT_PTHREAD(pthread_mutex_init(mutex, &attr));

        HE_ASSERT_PTHREAD(pthread_mutexattr_destroy(&attr));
    #else
        HE_ASSERT_PTHREAD(pthread_mutex_init(mutex, nullptr));
    #endif
    }

    Mutex::~Mutex() noexcept
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_mutex_destroy(mutex));
    }

    bool Mutex::TryAcquire()
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        const int r = pthread_mutex_trylock(mutex);
        HE_ASSERT(r == 0 || r == EBUSY, HE_KV(check_expr, "pthread_mutex_trylock(mutex)"), HE_KV(result, PosixResult(r)));
        return r == 0;
    }

    void Mutex::Acquire()
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_mutex_lock(mutex));
    }

    void Mutex::Release()
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_mutex_unlock(mutex));
    }

    // --------------------------------------------------------------------------------------------
    RecursiveMutex::RecursiveMutex() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(pthread_mutex_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(pthread_mutex_t)));

        pthread_mutexattr_t attr;
        HE_ASSERT_PTHREAD(pthread_mutexattr_init(&attr));
        HE_ASSERT_PTHREAD(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));

        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_mutex_init(mutex, &attr));

        HE_ASSERT_PTHREAD(pthread_mutexattr_destroy(&attr));
    }

    RecursiveMutex::~RecursiveMutex() noexcept
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_mutex_destroy(mutex));
    }

    bool RecursiveMutex::TryAcquire()
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        const int r = pthread_mutex_trylock(mutex);
        HE_ASSERT(r == 0 || r == EBUSY, HE_KV(check_expr, "pthread_mutex_trylock(mutex)"), HE_KV(result, PosixResult(r)));
        return r == 0;
    }

    void RecursiveMutex::Acquire()
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_mutex_lock(mutex));
    }

    void RecursiveMutex::Release()
    {
        pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_mutex_unlock(mutex));
    }

    // --------------------------------------------------------------------------------------------
    ConditionVariable::ConditionVariable() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(pthread_cond_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(pthread_cond_t)));

        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_cond_init(cv, nullptr));
    }

    ConditionVariable::~ConditionVariable() noexcept
    {
        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_cond_destroy(cv));
    }

    void ConditionVariable::WakeOne()
    {
        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_cond_signal(cv));
    }

    void ConditionVariable::WakeAll()
    {
        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_cond_broadcast(cv));
    }

    template <>
    void ConditionVariable::WaitMutex<Mutex>(Mutex& mutex)
    {
        pthread_mutex_t* m = reinterpret_cast<pthread_mutex_t*>(mutex.m_opaque);
        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_cond_wait(cv, m));
    }

    template <>
    void ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex)
    {
        pthread_mutex_t* m = reinterpret_cast<pthread_mutex_t*>(mutex.m_opaque);
        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        HE_ASSERT_PTHREAD(pthread_cond_wait(cv, m));
    }

    template <>
    bool ConditionVariable::WaitMutex<Mutex>(Mutex& mutex, Duration timeout)
    {
        const timespec timeoutSpec = PosixTimeFromDuration(timeout);
        pthread_mutex_t* m = reinterpret_cast<pthread_mutex_t*>(mutex.m_opaque);
        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        const int r = pthread_cond_timedwait(cv, m, &timeoutSpec);
        HE_ASSERT(r == 0 || r == ETIMEDOUT, HE_KV(check_expr, "pthread_cond_timedwait(cv, m, &timeoutSpec)"), HE_KV(result, PosixResult(r)));
        return r == 0;
    }

    template <>
    bool ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex, Duration timeout)
    {
        const timespec timeoutSpec = PosixTimeFromDuration(timeout);
        pthread_mutex_t* m = reinterpret_cast<pthread_mutex_t*>(mutex.m_opaque);
        pthread_cond_t* cv = reinterpret_cast<pthread_cond_t*>(m_opaque);
        const int r = pthread_cond_timedwait(cv, m, &timeoutSpec);
        HE_ASSERT(r == 0 || r == ETIMEDOUT, HE_KV(check_expr, "pthread_cond_timedwait(cv, m, &timeoutSpec)"), HE_KV(result, PosixResult(r)));
        return r == 0;
    }

    // --------------------------------------------------------------------------------------------
    Semaphore::Semaphore(uint32_t initialCount) noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(sem_t));
        HE_ASSERT(IsAligned(m_opaque, alignof(sem_t)));

        sem_t* sem = reinterpret_cast<sem_t*>(m_opaque);
        HE_ASSERT_ERRNO(sem_init(sem, 0, initialCount));
    }

    Semaphore::~Semaphore() noexcept
    {
        sem_t* sem = reinterpret_cast<sem_t*>(m_opaque);
        HE_ASSERT_ERRNO(sem_destroy(sem));
    }

    void Semaphore::Notify()
    {
        sem_t* sem = reinterpret_cast<sem_t*>(m_opaque);
        HE_ASSERT_ERRNO(sem_post(sem));
    }

    void Semaphore::Wait()
    {
        sem_t* sem = reinterpret_cast<sem_t*>(m_opaque);
        while (true)
        {
            const int r = sem_wait(sem);
            HE_ASSERT(r == 0 || errno == EINTR, HE_KV(check_expr, "sem_wait(sem)"), HE_KV(result, Result::FromLastError()));

            if (r == 0)
                return;
        }
    }

    bool Semaphore::Wait(Duration timeout)
    {
        sem_t* sem = reinterpret_cast<sem_t*>(m_opaque);

        if (timeout.val < 0)
            return false;

        if (timeout == Duration_Zero)
        {
            const int r = sem_trywait(sem);
            HE_ASSERT(r == 0 || errno == EAGAIN, HE_KV(check_expr, "sem_trywait(sem)"), HE_KV(result, Result::FromLastError()));
            return r == 0;
        }

        // TODO: handle potential overflow
        SystemTime end = SystemClock::Now();
        end += timeout;

        const struct timespec timeoutSpec = PosixTimeFromSystemTime(end);
        const int r = sem_timedwait(sem, &timeoutSpec);
        HE_ASSERT(r == 0 || errno == ETIMEDOUT, HE_KV(check_expr, "sem_timedwait(sem, timeoutSpec)"), HE_KV(result, Result::FromLastError()));

        return r == 0;
    }

    // --------------------------------------------------------------------------------------------
    struct SyncEventData
    {
    #if defined(__linux__)
        int eventFd{ -1 };
        int epollFd{ -1 };
    #else
        Mutex mutex{};
        ConditionVariable cv{};
        bool state{ false };
    #endif
        bool manualReset{ false };
    };

    SyncEvent::SyncEvent(bool manualReset, bool initiallySignaled) noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(SyncEventData));
        HE_ASSERT(IsAligned(m_opaque, alignof(SyncEventData)));

        SyncEventData* data = new(m_opaque) SyncEventData();
        data->manualReset = manualReset;

    #if defined(__linux__)
        data->eventFd = eventfd(0, EFD_CLOEXEC);
        HE_ASSERT(data->eventFd != -1, HE_KV(check_expr, "eventfd(0, EFD_CLOEXEC)"), HE_KV(result, Result::FromLastError()));

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = data->eventFd;

        data->epollFd = epoll_create(1);
        HE_ASSERT(data->epollFd != -1, HE_KV(check_expr, "epoll_create(1)"), HE_KV(result, Result::FromLastError()));

        HE_ASSERT_ERRNO(epoll_ctl(data->epollFd, EPOLL_CTL_ADD, data->eventFd, &ev));

        if (initiallySignaled)
            Signal();
    #else
        data->state = initiallySignaled;
    #endif
    }

    SyncEvent::~SyncEvent() noexcept
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);

    #if defined(__linux__)
        HE_ASSERT_ERRNO(close(data->eventFd));
        HE_ASSERT_ERRNO(close(data->epollFd));
    #else
        // Destructor is sufficient for this impl
    #endif

        data->~SyncEventData();
    }

    void SyncEvent::Signal()
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);

    #if defined(__linux__)
        uint64_t value = 1;
        const ssize_t nr = write(data->eventFd, &value, sizeof(uint64_t));
        HE_ASSERT(nr == sizeof(uint64_t),
            HE_KV(write_bytes, nr),
            HE_KV(check_expr, "write(data->eventFd, &value, sizeof(uint64_t))"),
            HE_KV(result, Result::FromLastError()));
    #else
        LockGuard lock(data->mutex);

        data->state = true;
        data->cv.WakeAll();
    #endif
    }

    void SyncEvent::Reset()
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);

    #if defined(__linux__)
        struct epoll_event ev;
        const ssize_t fdsReady = epoll_wait(data->epollFd, &ev, 1, 0);
        HE_ASSERT(fdsReady != -1,
            HE_KV(check_expr, "epoll_wait(data->epollFd, &ev, 1, 0)"),
            HE_KV(result, Result::FromLastError()));

        if (fdsReady > 0)
        {
            uint64_t value = 0;
            const ssize_t nr = read(data->eventFd, &value, sizeof(uint64_t));
            HE_ASSERT(nr == sizeof(uint64_t),
                HE_KV(read_bytes, nr),
                HE_KV(check_expr, "read(data->eventFd, &value, sizeof(uint64_t))"),
                HE_KV(result, Result::FromLastError()));
        }
    #else
        LockGuard lock(data->mutex);

        data->state = false;
    #endif
    }

    void SyncEvent::Wait()
    {
    #if defined(__linux__)
        Wait(Duration_Max);
    #else
        LockGuard lock(data->mutex);

        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);
        data->cv.Wait(data->mutex, [data]() { return data->state; });
        if (!data->manualReset)
            data->state = false;
    #endif
    }

    bool SyncEvent::Wait(Duration timeout)
    {
        SyncEventData* data = reinterpret_cast<SyncEventData*>(m_opaque);

    #if defined(__linux__)
        // When timeout < 0, epoll_wait() will wait indefinitely, but pthread_cond_timedwait()
        // will return ETIMEDOUT immediately so we do that here as well.
        if (timeout.val < 0)
            return false;

        const int timeoutMs = timeout == Duration_Max ? -1 : ToPeriod<Milliseconds, int>(timeout);

        struct epoll_event ev;
        const ssize_t fdsReady = epoll_wait(data->epollFd, &ev, 1, timeoutMs);
        HE_ASSERT(fdsReady != -1 || errno == EINTR,
            HE_KV(check_expr, "epoll_wait(data->epollFd, &ev, 1, timeoutMs)"),
            HE_KV(result, Result::FromLastError()));

        const bool result = fdsReady > 0;
        if (result && !data->manualReset)
            Reset();

        return result;
    #else
        LockGuard lock(data->mutex);

        const bool result = data->cv.Wait(data->mutex, [data]() { return data->state; }, timeout);
        if (result && !data->manualReset)
            data->state = false;

        return result;
    #endif
    }
}

#endif
