// Copyright Chad Engler

#pragma once

#include "platform_support.h"

#include "he/platform/debugger.h"

#if HE_CAN_PROVIDE_PLATFORM

namespace he
{
    class DebuggerImpl final : public Debugger
    {
    public:
        void Print(const char* s) const override;
        bool IsAttached() const override;
    };
}

#endif
