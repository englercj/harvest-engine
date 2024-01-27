// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/clock.h"
#include "he/core/sync.h"

#include <atomic>

#if defined(HE_PLATFORM_WASM)

#include "thread_internal.wasm.h"
#include "wasm/lib_core.wasm.h"

namespace he
{
    static std::atomic<uint32_t> s_nextThreadId{ 1 };
    static _WasmThreadState s_mainThreadState{};
    static IntrusiveList<_WasmThreadState, &_WasmThreadState::link> s_threadStates{};
    static Mutex s_threadStatesMutex{};

    static thread_local _WasmThreadState* s_localThreadState = nullptr;

    void _SetMainThreadState()
    {
        // Main thread cannot wait on the web, but on Node it can.
        _SetThreadState(&s_mainThreadState, true, !heWASM_IsWeb());
    }

    void _SetWorkerThreadState(_WasmThreadState* state)
    {
        // Worker threads can always wait.
        _SetThreadState(state, false, true);
    }

    void _SetThreadState(_WasmThreadState* state, bool isMain, bool canWait)
    {
        s_localThreadState = state;
        state->id = s_nextThreadId.fetch_add(1);
        state->isMain = isMain;
        state->canWait = canWait;

        s_threadStatesMutex.Lock();
        s_threadStates.PushBack(state);
        s_threadStatesMutex.Unlock();
    }

    bool _CanCurentThreadWait()
    {
        return s_localThreadState && s_localThreadState->canWait;
    }

    int32_t _AtomicWaitCurrentThread(int32_t* value, int32_t expected, int64_t timeoutNs)
    {
        // memory.atomic.wait32 returns:
        //   0 => "ok", woken by another agent.
        //   1 => "not-equal", loaded value != expected value
        //   2 => "timed-out", the timeout expired
        if (_CanCurentThreadWait())
        {
            return __builtin_wasm_atomic_wait_i32(value, expected, timeoutNs);
        }

        return *value == expected ? 0 : 1;
    }

    uintptr_t GetCurrentThreadHandle()
    {
        return reinterpret_cast<uintptr_t>(s_localThreadState);
    }

    uint32_t GetCurrentThreadId()
    {
        return s_localThreadState ? s_localThreadState->id : 0;
    }

    Result SetThreadAffinity(uintptr_t thread, uint64_t mask)
    {
        // Wasm doesn't have thread affinity support.
        return Result::NotSupported;
    }

    void SetCurrentThreadName(const char* name)
    {
        if (s_localThreadState)
        {
            s_localThreadState->name = name;
        }
    }

    void SleepCurrentThread(Duration amount)
    {
        static int32_t s_dummy = 0;
        _AtomicWaitCurrentThread(&s_dummy, 0, amount.val);
    }

    void YieldCurrentThread()
    {
        static int32_t s_dummy = 0;
        _AtomicWaitCurrentThread(&s_dummy, 0, 100);
    }
}

#endif
