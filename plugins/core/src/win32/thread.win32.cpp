// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <cstdlib>
#include <fibersapi.h>
#include <process.h>

#undef Yield

namespace he
{
    uint32_t Thread::GetId()
    {
        static_assert(sizeof(uint32_t) >= sizeof(DWORD));
        return static_cast<uint32_t>(::GetCurrentThreadId());
    }

    void Thread::SetName(const char* name)
    {
        ::SetThreadDescription(::GetCurrentThread(), HE_TO_WCSTR(name));
    }

    void Thread::Sleep(Duration amount)
    {
        const uint32_t ms = ToPeriod<Milliseconds, uint32_t>(amount);
        ::Sleep(ms);
    }

    void Thread::Yield()
    {
        ::SwitchToThread();
    }

    struct _ThreadThunkData
    {
        Pfn_ThreadProc proc;
        void* userData;
    };

    unsigned __stdcall _ThreadThunk(void* data)
    {
        _ThreadThunkData* thunkData = static_cast<_ThreadThunkData*>(data);
        thunkData->proc(thunkData->userData);
        Allocator::GetDefault().Delete(thunkData);
        return 0;
    }

    Result Thread::Start(const ThreadDesc& desc)
    {
        if (!HE_VERIFY(desc.proc != nullptr, HE_MSG("Thread procedure must be non-null.")))
            return Result::InvalidParameter;

        if (!HE_VERIFY(!m_handle, HE_MSG("Thread is already running.")))
            return Result::InvalidParameter;

        _ThreadThunkData* thunkData = Allocator::GetDefault().New<_ThreadThunkData>(desc.proc, desc.data);
        m_handle = reinterpret_cast<void*>(::_beginthreadex(nullptr, desc.stackSize, _ThreadThunk, thunkData, 0, nullptr));

        if (!m_handle)
        {
            Allocator::GetDefault().Delete(thunkData);
            return Win32Result(_doserrno);
        }

        if (desc.affinity)
        {
            // Ignore errors when setting the affinity of the newly created thread.
            SetAffinity(desc.affinity);
        }

        return Result::Success;
    }

    Result Thread::Join()
    {
        if (::WaitForSingleObjectEx(m_handle, INFINITE, FALSE) == WAIT_FAILED)
            return Win32Result(::GetLastError());

        if (!::CloseHandle(m_handle))
            return Win32Result(::GetLastError());

        m_handle = nullptr;
        return Result::Success;
    }

    Result Thread::Detach()
    {
        if (!::CloseHandle(m_handle))
            return Win32Result(::GetLastError());

        m_handle = nullptr;
        return Result::Success;
    }

    Result Thread::SetAffinity(uint64_t mask)
    {
        DWORD_PTR processMask = 0;
        DWORD_PTR systemMask = 0;
        if (!::GetProcessAffinityMask(::GetCurrentProcess(), &processMask, &systemMask))
            return Result::FromLastError();

        mask &= processMask;

        if (!HE_VERIFY(mask > 0, HE_MSG("Affinity mask shouldn't be zero.")))
            return Result::InvalidParameter;

        if (!::SetThreadAffinityMask(m_handle, mask))
            return Result::FromLastError();

        return Result::Success;
    }

    uintptr_t TlsValue::InvalidId = FLS_OUT_OF_INDEXES;

    Result TlsValue::Create(Pfn_TlsDestructor destroy) noexcept
    {
        m_id = ::FlsAlloc(destroy);
        return m_id == InvalidId ? Result::FromLastError() : Result::Success;
    }

    void TlsValue::Destroy() noexcept
    {
        if (m_id != InvalidId)
        {
            ::FlsFree(m_id);
            m_id = InvalidId;
        }
    }

    void* TlsValue::Get() const noexcept
    {
        return m_id == InvalidId ? nullptr : ::FlsGetValue(m_id);
    }

    void TlsValue::Set(void* value) noexcept
    {
        if (m_id != InvalidId)
        {
            ::FlsSetValue(m_id, value);
        }
    }
}

#endif
