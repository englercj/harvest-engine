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
                case KeyValue::ValueKind::Bool: FormatTo(out, "{} = {}", kv.Key(), kv.Bool()); break;
                case KeyValue::ValueKind::Int: FormatTo(out, "{} = {}", kv.Key(), kv.Int()); break;
                case KeyValue::ValueKind::Uint: FormatTo(out, "{} = {}", kv.Key(), kv.Uint()); break;
                case KeyValue::ValueKind::Double: FormatTo(out, "{} = {}", kv.Key(), kv.Double()); break;
                case KeyValue::ValueKind::String: FormatTo(out, "{} = {}", kv.Key(), kv.String().Data()); break;

                case KeyValue::ValueKind::Enum:
                {
                    const KeyValue::EnumStorage& e = kv.Enum();
                    FormatTo(out, "{} = {}({})", kv.Key(), e.String(), e.value);
                    break;
                }

                case KeyValue::ValueKind::Empty:
                    break;
            }
        }
    };
}
