// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/system.h"
#include "he/core/string_fmt.h"

namespace he
{
    template <>
    struct Formatter<SystemInfo>
    {
        using Type = SystemInfo;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const SystemInfo& info) const
        {
            return FormatTo(out, "{} ({}.{}.{}.{})",
                info.platform, info.version.major, info.version.minor, info.version.patch, info.version.build);
        }
    };
}
