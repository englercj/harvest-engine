// Copyright Chad Engler

#pragma once

#include "he/core/atomic.h"
#include "he/core/fmt.h"
#include "he/core/type_traits.h"

namespace he
{
    template <typename T>
    struct Formatter<Atomic<T>>
    {
        using Type = Atomic<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Atomic<T>& atom) const
        {
            if constexpr (IsPointer<T>)
                return FormatTo(out, "{}", FmtPtr(atom.Load(MemoryOrder::Relaxed)));
            else
                return FormatTo(out, "{}", atom.Load(MemoryOrder::Relaxed));
        }
    };
}
