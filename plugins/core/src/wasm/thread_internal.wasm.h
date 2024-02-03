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
        bool isMain{ false };
        bool canWait{ false };

        void* memBase{ nullptr };
        void* tlsBase{ nullptr };
        void* stackBase{ nullptr };
        size_t memSize{ 0 };
    };

    bool _CanCurentThreadWait();
    int32_t _AtomicWaitCurrentThread(int32_t* value, int32_t expected, int64_t timeoutNs);
    int32_t _AtomicWaitCurrentThread64(int64_t* value, int64_t expected, int64_t timeoutNs);
}
