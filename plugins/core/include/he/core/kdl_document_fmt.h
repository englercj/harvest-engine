// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/kdl_document.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/variant_fmt.h"

namespace he
{
    template <>
    struct Formatter<KdlValue>
    {
        using Type = KdlValue;

        constexpr const char* Parse(const FmtParseCtx& ctx) { return ctx.Begin(); }

        void Format(String& out, const Type& value) const
        {
            if (value.Type())
            {
                FormatTo(out, "({}){}", *value.Type(), value.Value());
            }
            else
            {
                FormatTo(out, "{}", value.Value());
            }
        }
    };

    template <>
    struct Formatter<KdlDocument>
    {
        using Type = KdlDocument;

        constexpr const char* Parse(const FmtParseCtx& ctx) { return ctx.Begin(); }

        void Format(String& out, const Type& value) const
        {
            value.Write(out);
        }
    };
}
