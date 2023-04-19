// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/string_pool.h"

namespace he
{
    template <>
    struct Formatter<StringPoolId>
    {
        using Type = StringPoolId;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, StringPoolId value) const
        {
            FormatTo(out, "{}", value.val);
        }
    };
}
