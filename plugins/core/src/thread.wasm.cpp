// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/clock.h"

#if defined(HE_PLATFORM_WASM)

#include "thread_internal.wasm.h"
#include "wasm_core.js.h"

namespace he
{
    static WasmThreadState s_mainThreadState{};
    static IntrusiveList<WasmThreadState, &WasmThreadState::link> s_threadStates{};

    static thread_local WasmThreadState* s_localThreadState = nullptr;

    void _SetMainThreadState()
    {
        // Main thread cannot block on the web, but on Node it can.
        _SetThreadState(&s_mainThreadState, true, !heWASM_IsWeb());
    }

    void _SetWorkerThreadState(WasmThreadState* state)
    {
        // Worker threads can always block.
        _SetThreadState(state, false, true);
    }

    void _SetThreadState(WasmThreadState* state, bool isMain, bool canBlock)
    {
        s_localThreadState = state;
        state->isMain = isMain;
        state->canBlock = canBlock;

        // TODO: lock access here.
        s_threadStates.PushBack(state);
    }

    ThreadHandle GetCurrentThreadHandle()
    {
        return reinterpret_cast<ThreadHandle>(s_localThreadState);
    }

    Result SetThreadAffinity(ThreadHandle thread, uint64_t mask)
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
        if (s_localThreadState && s_localThreadState->canBlock)
        {
            static int s_dummy = 0;
            __builtin_wasm_memory_atomic_wait32(&s_dummy, 0, amount.val);
        }
    }

    void YieldCurrentThread()
    {
        // Wasm doesn't really have a concept of yielding.
    }
}

#endif
