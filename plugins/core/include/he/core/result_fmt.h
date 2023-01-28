// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/result.h"

namespace he
{
    template <>
    struct Formatter<Result>
    {
        using Type = Result;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Result& result) const
        {
            FormatTo(out, "({}) ", result.GetCode());
            result.ToString(out);
        }
    };
}
