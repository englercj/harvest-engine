// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/unique_ptr.h"

namespace he
{
    template <typename T>
    struct Formatter<UniquePtr<T>>
    {
        using Type = UniquePtr<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) { return ctx.Begin(); }

        void Format(String& out, const UniquePtr<T>& value) const
        {
            FormatTo(out, "{}", FmtPtr(value.Get()));
        }
    };
}
