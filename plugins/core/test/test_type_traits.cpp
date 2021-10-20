// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/type_traits.h"

#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/vector.h"

#include <string>
#include <string_view>
#include <vector>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, StdContiguousRange)
{
    static_assert(StdContiguousRange<std::string, const char>);
    static_assert(StdContiguousRange<std::string_view, const char>);
    static_assert(StdContiguousRange<std::vector<int>, int>);

    static_assert(!StdContiguousRange<std::string, int>);
    static_assert(!StdContiguousRange<std::string_view, char>);
    static_assert(!StdContiguousRange<std::vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ContiguousRange)
{
    static_assert(ContiguousRange<String, char>);
    static_assert(ContiguousRange<StringView, const char>);
    static_assert(ContiguousRange<Vector<int>, int>);

    static_assert(!ContiguousRange<String, int>);
    static_assert(!ContiguousRange<StringView, char>);
    static_assert(!ContiguousRange<Vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsSpecialization)
{
    static_assert(IsSpecialization<std::vector<int>, std::vector>);
    static_assert(IsSpecialization<Vector<int>, Vector>);
    static_assert(!IsSpecialization<Vector<int>, std::vector>);
}
