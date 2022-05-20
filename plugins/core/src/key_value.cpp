// Copyright Chad Engler

#include "he/core/key_value.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"

namespace he
{
    bool KeyValue::GetBool() const
    {
        HE_ASSERT(m_kind == ValueKind::Bool);
        return m_value.b;
    }

    int64_t KeyValue::GetInt() const
    {
        HE_ASSERT(m_kind == ValueKind::Int);
        return m_value.i;
    }

    uint64_t KeyValue::GetUint() const
    {
        HE_ASSERT(m_kind == ValueKind::Uint);
        return m_value.u;
    }

    double KeyValue::GetDouble() const
    {
        HE_ASSERT(m_kind == ValueKind::Double);
        return m_value.d;
    }

    const String& KeyValue::GetString() const
    {
        HE_ASSERT(m_kind == ValueKind::String);
        return m_value.s;
    }

    uint64_t KeyValue::GetEnumValue() const
    {
        HE_ASSERT(m_kind == ValueKind::Enum);
        return m_value.e.value;
    }

    const char* KeyValue::GetEnumString() const
    {
        HE_ASSERT(m_kind == ValueKind::Enum);
        return m_value.e.toString(m_value.e.value);
    }

    template <>
    const char* AsString(KeyValue::ValueKind x)
    {
        switch (x)
        {
            case KeyValue::ValueKind::Bool: return "Bool";
            case KeyValue::ValueKind::Enum: return "Enum";
            case KeyValue::ValueKind::Int: return "Int";
            case KeyValue::ValueKind::Uint: return "Uint";
            case KeyValue::ValueKind::Double: return "Double";
            case KeyValue::ValueKind::String: return "String";
        }

        return "<unknown>";
    }
}
