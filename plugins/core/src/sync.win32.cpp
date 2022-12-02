// Copyright Chad Engler

#include "he/core/sync.h"

#include "he/core/assert.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/utils.h"

#include <condition_variable>

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    RWLock::RWLock() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(SRWLOCK));
        HE_ASSERT(IsAligned(m_opaque, alignof(SRWLOCK)));

        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::InitializeSRWLock(srw);
    }

    RWLock::~RWLock() noexcept
    {
        HE_ASSERT(TryAcquireWrite() && (ReleaseWrite(), true));
    }

    bool RWLock::TryAcquireRead()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        return ::TryAcquireSRWLockShared(srw) != 0;
    }

    void RWLock::AcquireRead()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::AcquireSRWLockShared(srw);
    }

    void RWLock::ReleaseRead()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::ReleaseSRWLockShared(srw);
    }

    bool RWLock::TryAcquireWrite()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        return ::TryAcquireSRWLockExclusive(srw) != 0;
    }

    void RWLock::AcquireWrite()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::AcquireSRWLockExclusive(srw);
    }

    void RWLock::ReleaseWrite()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::ReleaseSRWLockExclusive(srw);
    }

    // --------------------------------------------------------------------------------------------
    Mutex::Mutex() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(SRWLOCK));
        HE_ASSERT(IsAligned(m_opaque, alignof(SRWLOCK)));

        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::InitializeSRWLock(srw);
    }

    Mutex::~Mutex() noexcept
    {
        HE_ASSERT(TryAcquire() && (Release(), true));
    }

    bool Mutex::TryAcquire()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        return ::TryAcquireSRWLockExclusive(srw) != 0;
    }

    void Mutex::Acquire()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::AcquireSRWLockExclusive(srw);
    }

    void Mutex::Release()
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(m_opaque);
        ::ReleaseSRWLockExclusive(srw);
    }

    // --------------------------------------------------------------------------------------------
    RecursiveMutex::RecursiveMutex() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(CRITICAL_SECTION));
        HE_ASSERT(IsAligned(m_opaque, alignof(CRITICAL_SECTION)));

        CRITICAL_SECTION* cs = reinterpret_cast<CRITICAL_SECTION*>(m_opaque);
        ::InitializeCriticalSection(cs);
    }

    RecursiveMutex::~RecursiveMutex() noexcept {}

    bool RecursiveMutex::TryAcquire()
    {
        CRITICAL_SECTION* cs = reinterpret_cast<CRITICAL_SECTION*>(m_opaque);
        return ::TryEnterCriticalSection(cs) != 0;
    }

    void RecursiveMutex::Acquire()
    {
        CRITICAL_SECTION* cs = reinterpret_cast<CRITICAL_SECTION*>(m_opaque);
        ::EnterCriticalSection(cs);
    }

    void RecursiveMutex::Release()
    {
        CRITICAL_SECTION* cs = reinterpret_cast<CRITICAL_SECTION*>(m_opaque);
        ::LeaveCriticalSection(cs);
    }

    // --------------------------------------------------------------------------------------------
    ConditionVariable::ConditionVariable() noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(CONDITION_VARIABLE));
        HE_ASSERT(IsAligned(m_opaque, alignof(CONDITION_VARIABLE)));

        CONDITION_VARIABLE* cv = reinterpret_cast<CONDITION_VARIABLE*>(m_opaque);
        ::InitializeConditionVariable(cv);
    }

    ConditionVariable::~ConditionVariable() noexcept
    {

    }

    void ConditionVariable::WakeOne()
    {
        CONDITION_VARIABLE* cv = reinterpret_cast<CONDITION_VARIABLE*>(m_opaque);
        ::WakeConditionVariable(cv);
    }

    void ConditionVariable::WakeAll()
    {
        CONDITION_VARIABLE* cv = reinterpret_cast<CONDITION_VARIABLE*>(m_opaque);
        ::WakeAllConditionVariable(cv);
    }

    template <>
    void ConditionVariable::WaitMutex<Mutex>(Mutex& mutex)
    {
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(mutex.m_opaque);
        CONDITION_VARIABLE* cv = reinterpret_cast<CONDITION_VARIABLE*>(m_opaque);
        const BOOL r = ::SleepConditionVariableSRW(cv, srw, INFINITE, 0);
        HE_ASSERT(r != 0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    template <>
    void ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex)
    {
        CRITICAL_SECTION* cs = reinterpret_cast<CRITICAL_SECTION*>(mutex.m_opaque);
        CONDITION_VARIABLE* cv = reinterpret_cast<CONDITION_VARIABLE*>(m_opaque);
        const BOOL r = ::SleepConditionVariableCS(cv, cs, INFINITE);
        HE_ASSERT(r != 0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    template <>
    bool ConditionVariable::WaitMutex<Mutex>(Mutex& mutex, Duration timeout)
    {
        const uint32_t timeoutMs = ToPeriod<Milliseconds, uint32_t>(timeout);
        SRWLOCK* srw = reinterpret_cast<SRWLOCK*>(mutex.m_opaque);
        CONDITION_VARIABLE* cv = reinterpret_cast<CONDITION_VARIABLE*>(m_opaque);
        return ::SleepConditionVariableSRW(cv, srw, timeoutMs, 0) != 0;
    }

    template <>
    bool ConditionVariable::WaitMutex<RecursiveMutex>(RecursiveMutex& mutex, Duration timeout)
    {
        const uint32_t timeoutMs = ToPeriod<Milliseconds, uint32_t>(timeout);
        CRITICAL_SECTION* cs = reinterpret_cast<CRITICAL_SECTION*>(mutex.m_opaque);
        CONDITION_VARIABLE* cv = reinterpret_cast<CONDITION_VARIABLE*>(m_opaque);
        return ::SleepConditionVariableCS(cv, cs, timeoutMs) != 0;
    }

    // --------------------------------------------------------------------------------------------
    Semaphore::Semaphore(uint32_t initialCount) noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(HANDLE));
        HE_ASSERT(IsAligned(m_opaque, alignof(HANDLE)));

        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        h = ::CreateSemaphoreW(nullptr, initialCount, INT_MAX, nullptr);
        HE_ASSERT(h != nullptr, HE_KV(result, Result::FromLastError()));
    }

    Semaphore::~Semaphore() noexcept
    {
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const BOOL r = ::CloseHandle(h);
        HE_ASSERT(r != 0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    void Semaphore::Notify()
    {
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const BOOL r = ::ReleaseSemaphore(h, 1, nullptr);
        HE_ASSERT(r != 0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    void Semaphore::Wait()
    {
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const DWORD r = ::WaitForSingleObject(h, INFINITE);
        HE_ASSERT(r == WAIT_OBJECT_0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    bool Semaphore::Wait(Duration timeout)
    {
        const uint32_t timeoutMs = ToPeriod<Milliseconds, uint32_t>(timeout);
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const DWORD r = ::WaitForSingleObject(h, timeoutMs);
        HE_ASSERT(r == WAIT_OBJECT_0 || r == WAIT_TIMEOUT, HE_KV(result, Result::FromLastError()));
        return r == WAIT_OBJECT_0;
    }

    // --------------------------------------------------------------------------------------------
    SyncEvent::SyncEvent(bool manualReset, bool initiallySignaled) noexcept
    {
        static_assert(sizeof(m_opaque) == sizeof(HANDLE));
        HE_ASSERT(IsAligned(m_opaque, alignof(HANDLE)));

        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        h = ::CreateEventW(nullptr, manualReset, initiallySignaled, nullptr);
    }

    SyncEvent::~SyncEvent() noexcept
    {
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const BOOL r = ::CloseHandle(h);
        HE_ASSERT(r != 0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    void SyncEvent::Signal()
    {
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const BOOL r = ::SetEvent(h);
        HE_ASSERT(r != 0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    void SyncEvent::Reset()
    {
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const BOOL r = ::ResetEvent(h) != 0;
        HE_ASSERT(r != 0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    void SyncEvent::Wait()
    {
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const DWORD r = ::WaitForSingleObject(h, INFINITE);
        HE_ASSERT(r == WAIT_OBJECT_0, HE_KV(result, Result::FromLastError()));
        HE_UNUSED(r);
    }

    bool SyncEvent::Wait(Duration timeout)
    {
        const uint32_t timeoutMs = ToPeriod<Milliseconds, uint32_t>(timeout);
        HANDLE& h = *reinterpret_cast<HANDLE*>(&m_opaque);
        const DWORD r = ::WaitForSingleObject(h, timeoutMs);
        HE_ASSERT(r == WAIT_OBJECT_0 || r == WAIT_TIMEOUT, HE_KV(result, Result::FromLastError()));
        return r == WAIT_OBJECT_0;
    }
}

#endif
