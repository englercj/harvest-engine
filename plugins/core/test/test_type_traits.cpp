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
HE_TEST(core, type_traits, ProvidesStdContiguousRange)
{
    static_assert(ProvidesStdContiguousRange<std::string, const char>);
    static_assert(ProvidesStdContiguousRange<std::string_view, const char>);
    static_assert(ProvidesStdContiguousRange<std::vector<int>, int>);

    static_assert(!ProvidesStdContiguousRange<std::string, int>);
    static_assert(!ProvidesStdContiguousRange<std::string_view, char>);
    static_assert(!ProvidesStdContiguousRange<std::vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ProvidesContiguousRange)
{
    static_assert(ProvidesContiguousRange<String, char>);
    static_assert(ProvidesContiguousRange<StringView, const char>);
    static_assert(ProvidesContiguousRange<Vector<int>, int>);

    static_assert(!ProvidesContiguousRange<String, int>);
    static_assert(!ProvidesContiguousRange<StringView, char>);
    static_assert(!ProvidesContiguousRange<Vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsSpecialization)
{
    static_assert(IsSpecialization<std::vector<int>, std::vector>);
    static_assert(IsSpecialization<Vector<int>, Vector>);
    static_assert(!IsSpecialization<Vector<int>, std::vector>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsEnum)
{
    enum TestEnum { A };
    static_assert(IsEnum<TestEnum>);
    static_assert(!IsEnum<int>);
    static_assert(!IsEnum<Vector<int>>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, EnumType)
{
    enum class TestEnumInt { A };
    enum class TestEnumUint : uint32_t { A };

    static_assert(std::is_same<EnumType<TestEnumInt>, int32_t>::value);
    static_assert(std::is_same<EnumType<TestEnumUint>, uint32_t>::value);

    static_assert(!std::is_same<EnumType<TestEnumInt>, bool>::value);
    static_assert(!std::is_same<EnumType<TestEnumUint>, int8_t>::value);
}
