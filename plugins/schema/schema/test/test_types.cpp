// Copyright Chad Engler

#include "he/schema/types.h"

#include "he/core/test.h"

#include <type_traits>

using namespace he::schema;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, types, TypeId)
{
    static_assert(std::is_same_v<TypeId, uint64_t>);
    static_assert(TypeIdFlag == (1ull << 63));

    TypeIdHasher h;
    HE_EXPECT_EQ(h(123 | TypeIdFlag), (123 | TypeIdFlag));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, types, Word)
{
    static_assert(std::is_same_v<Word, uint64_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, types, Void)
{
    static_assert(Void{} == Void{});
    static_assert(!(Void{} != Void{}));

    HE_EXPECT(Void{} == Void{});
    HE_EXPECT(!(Void{} != Void{}));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, types, PointerKind)
{
    // Changing any of these values is a breaking change to the binary format, so check them here
    static_assert(PointerKind::Struct == PointerKind(0));
    static_assert(PointerKind::List == PointerKind(1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, types, ElementSize)
{
    // Changing any of these values is a breaking change to the binary format, so check them here
    static_assert(ElementSize::Void == ElementSize(0));
    static_assert(ElementSize::Bit == ElementSize(1));
    static_assert(ElementSize::Byte == ElementSize(2));
    static_assert(ElementSize::TwoBytes == ElementSize(3));
    static_assert(ElementSize::FourBytes == ElementSize(4));
    static_assert(ElementSize::EightBytes == ElementSize(5));
    static_assert(ElementSize::Pointer == ElementSize(6));
    static_assert(ElementSize::Composite == ElementSize(7));
}

// ------------------------------------------------------------------------------------------------
HE_SCHEMA_DECL_INFO(0x86387955f9f51cad, 0x2503e27f9b0730f1, Struct, 1, 2, 3);
HE_TEST(schema, types, DeclInfo)
{
    static_assert(DeclInfo<0x86387955f9f51cad>::Id == 0x86387955f9f51cad);
    static_assert(DeclInfo<0x86387955f9f51cad>::ParentId == 0x2503e27f9b0730f1);
    static_assert(DeclInfo<0x86387955f9f51cad>::Kind == DeclKind::Struct);
    static_assert(DeclInfo<0x86387955f9f51cad>::DataFieldCount == 1);
    static_assert(DeclInfo<0x86387955f9f51cad>::DataWordSize == 2);
    static_assert(DeclInfo<0x86387955f9f51cad>::PointerCount == 3);
    static_assert(std::is_same_v<decltype(DeclInfo<0x86387955f9f51cad>::DefaultValue), const Word*>);
    static_assert(std::is_same_v<decltype(DeclInfo<0x86387955f9f51cad>::RawSchema), const Word*>);
}
