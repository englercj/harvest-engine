// Copyright Chad Engler

#pragma once

#include "he/core/intrusive_list.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    struct _WasmThreadState
    {
        IntrusiveListLink<_WasmThreadState> link{};

        String name{};
        uint32_t id{ 0 };
        bool isMain : 1{ false };
        bool canWait : 1{ false };
    };

    void _SetMainThreadState();
    void _SetWorkerThreadState(_WasmThreadState* state);
    void _SetThreadState(_WasmThreadState* state, bool isMain, bool canWait);

    bool _CanCurentThreadWait();
    int32_t _AtomicWaitCurrentThread(int32_t* value, int32_t expected, int64_t timeoutNs);
}
