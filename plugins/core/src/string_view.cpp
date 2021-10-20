// Copyright Chad Engler

#include "he/core/string_view.h"

#include "he/core/hash.h"

namespace std
{
    size_t hash<he::StringView>::operator()(const he::StringView& value) const
    {
    #if HE_CPU_64_BIT
        return he::FNV64::HashData(value.Data(), value.Size());
    #else
        return he::FNV32::HashData(value.Data(), value.Size());
    #endif
    }
}
