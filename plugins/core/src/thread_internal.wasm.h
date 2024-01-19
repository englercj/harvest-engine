// Copyright Chad Engler

#pragma once

#include "he/core/intrusive_list.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    struct WasmThreadState
    {
        IntrusiveListLink<WasmThreadState> link{};

        String name{};
        bool isMain : 1{ false };
        bool canBlock : 1{ false };
    };

    void _SetMainThreadState();
    void _SetWorkerThreadState(WasmThreadState* state);
    void _SetThreadState(WasmThreadState* state, bool isMain, bool canBlock);
}
