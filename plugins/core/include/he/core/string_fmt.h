// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/string.h"

namespace he
{
    template <> struct Formatter<String> : Formatter<StringView> {};
}
