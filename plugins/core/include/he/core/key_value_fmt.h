// Copyright Chad Engler

#pragma once

#include "he/core/key_value.h"

#include "fmt/core.h"

namespace fmt
{
    template <>
    struct formatter<he::KeyValue>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::KeyValue& kv, FormatContext& ctx) const -> decltype(ctx.out())
        {
            constexpr const char ValueFmt[] = "{} = {}";

            switch (kv.Kind())
            {
                case he::KeyValue::ValueKind::Bool: return fmt::format_to(ctx.out(), ValueFmt, kv.Key(), kv.GetBool());
                case he::KeyValue::ValueKind::Enum: return fmt::format_to(ctx.out(), "{} = {}({})", kv.Key(), kv.GetEnumString(), kv.GetEnumValue());
                case he::KeyValue::ValueKind::Int: return fmt::format_to(ctx.out(), ValueFmt, kv.Key(), kv.GetInt());
                case he::KeyValue::ValueKind::Uint: return fmt::format_to(ctx.out(), ValueFmt, kv.Key(), kv.GetUint());
                case he::KeyValue::ValueKind::Double: return fmt::format_to(ctx.out(), ValueFmt, kv.Key(), kv.GetDouble());
                case he::KeyValue::ValueKind::String: return fmt::format_to(ctx.out(), ValueFmt, kv.Key(), kv.GetString().Data());
            }
            return ctx.out();
        }
    };
}
