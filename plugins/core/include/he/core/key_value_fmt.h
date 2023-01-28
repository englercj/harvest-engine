// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/key_value.h"

namespace he
{
    template <>
    struct Formatter<KeyValue>
    {
        using Type = KeyValue;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const KeyValue& kv) const
        {
            switch (kv.Kind())
            {
                case KeyValue::ValueKind::Bool: FormatTo(out, "{} = {}", kv.Key(), kv.GetBool()); break;
                case KeyValue::ValueKind::Enum: FormatTo(out, "{} = {}({})", kv.Key(), kv.GetEnumString(), kv.GetEnumValue()); break;
                case KeyValue::ValueKind::Int: FormatTo(out, "{} = {}", kv.Key(), kv.GetInt()); break;
                case KeyValue::ValueKind::Uint: FormatTo(out, "{} = {}", kv.Key(), kv.GetUint()); break;
                case KeyValue::ValueKind::Double: FormatTo(out, "{} = {}", kv.Key(), kv.GetDouble()); break;
                case KeyValue::ValueKind::String: FormatTo(out, "{} = {}", kv.Key(), kv.GetString().Data()); break;
            }
        }
    };
}
