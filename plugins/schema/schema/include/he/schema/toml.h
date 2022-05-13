// Copyright Chad Engler

#pragma once

#include "he/core/string_builder.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"

namespace he::schema
{
    bool ToToml(StringBuilder& dst, StructReader data, const DeclInfo& info);

    template <typename T>
    bool ToToml(StringBuilder& dst, typename T::Reader data) { return ToToml(dst, data, T::DeclInfo); }

    bool FromToml(Builder& dst, const char* data, const DeclInfo& info);

    template <typename T>
    bool FromToml(Builder& dst, const char* data) { return FromToml(dst, data, T::DeclInfo); }
}
