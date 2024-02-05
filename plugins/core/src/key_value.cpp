// Copyright Chad Engler

#include "he/core/key_value.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"

namespace he
{
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
            case KeyValue::ValueKind::Empty: return "Empty";
        }

        return "<unknown>";
    }
}
