// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/allocator.h"
#include "he/core/test.h"

using namespace he;
using namespace he::schema;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsSignedIntegral)
{
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Void));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Bool));
    static_assert(IsSignedIntegral(Type::Data::UnionTag::Int8));
    static_assert(IsSignedIntegral(Type::Data::UnionTag::Int16));
    static_assert(IsSignedIntegral(Type::Data::UnionTag::Int32));
    static_assert(IsSignedIntegral(Type::Data::UnionTag::Int64));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Uint8));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Uint16));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Uint32));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Uint64));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Float32));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Float64));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Blob));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::String));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Array));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::List));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Enum));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Struct));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::Interface));
    static_assert(!IsSignedIntegral(Type::Data::UnionTag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsUnsignedIntegral)
{
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Void));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Bool));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Int8));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Int16));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Int32));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Int64));
    static_assert(IsUnsignedIntegral(Type::Data::UnionTag::Uint8));
    static_assert(IsUnsignedIntegral(Type::Data::UnionTag::Uint16));
    static_assert(IsUnsignedIntegral(Type::Data::UnionTag::Uint32));
    static_assert(IsUnsignedIntegral(Type::Data::UnionTag::Uint64));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Float32));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Float64));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Blob));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::String));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Array));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::List));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Enum));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Struct));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::Interface));
    static_assert(!IsUnsignedIntegral(Type::Data::UnionTag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsIntegral)
{
    static_assert(!IsIntegral(Type::Data::UnionTag::Void));
    static_assert(!IsIntegral(Type::Data::UnionTag::Bool));
    static_assert(IsIntegral(Type::Data::UnionTag::Int8));
    static_assert(IsIntegral(Type::Data::UnionTag::Int16));
    static_assert(IsIntegral(Type::Data::UnionTag::Int32));
    static_assert(IsIntegral(Type::Data::UnionTag::Int64));
    static_assert(IsIntegral(Type::Data::UnionTag::Uint8));
    static_assert(IsIntegral(Type::Data::UnionTag::Uint16));
    static_assert(IsIntegral(Type::Data::UnionTag::Uint32));
    static_assert(IsIntegral(Type::Data::UnionTag::Uint64));
    static_assert(!IsIntegral(Type::Data::UnionTag::Float32));
    static_assert(!IsIntegral(Type::Data::UnionTag::Float64));
    static_assert(!IsIntegral(Type::Data::UnionTag::Blob));
    static_assert(!IsIntegral(Type::Data::UnionTag::String));
    static_assert(!IsIntegral(Type::Data::UnionTag::Array));
    static_assert(!IsIntegral(Type::Data::UnionTag::List));
    static_assert(!IsIntegral(Type::Data::UnionTag::Enum));
    static_assert(!IsIntegral(Type::Data::UnionTag::Struct));
    static_assert(!IsIntegral(Type::Data::UnionTag::Interface));
    static_assert(!IsIntegral(Type::Data::UnionTag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsFloat)
{
    static_assert(!IsFloat(Type::Data::UnionTag::Void));
    static_assert(!IsFloat(Type::Data::UnionTag::Bool));
    static_assert(!IsFloat(Type::Data::UnionTag::Int8));
    static_assert(!IsFloat(Type::Data::UnionTag::Int16));
    static_assert(!IsFloat(Type::Data::UnionTag::Int32));
    static_assert(!IsFloat(Type::Data::UnionTag::Int64));
    static_assert(!IsFloat(Type::Data::UnionTag::Uint8));
    static_assert(!IsFloat(Type::Data::UnionTag::Uint16));
    static_assert(!IsFloat(Type::Data::UnionTag::Uint32));
    static_assert(!IsFloat(Type::Data::UnionTag::Uint64));
    static_assert(IsFloat(Type::Data::UnionTag::Float32));
    static_assert(IsFloat(Type::Data::UnionTag::Float64));
    static_assert(!IsFloat(Type::Data::UnionTag::Blob));
    static_assert(!IsFloat(Type::Data::UnionTag::String));
    static_assert(!IsFloat(Type::Data::UnionTag::Array));
    static_assert(!IsFloat(Type::Data::UnionTag::List));
    static_assert(!IsFloat(Type::Data::UnionTag::Enum));
    static_assert(!IsFloat(Type::Data::UnionTag::Struct));
    static_assert(!IsFloat(Type::Data::UnionTag::Interface));
    static_assert(!IsFloat(Type::Data::UnionTag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsArithmetic)
{
    static_assert(!IsArithmetic(Type::Data::UnionTag::Void));
    static_assert(!IsArithmetic(Type::Data::UnionTag::Bool));
    static_assert(IsArithmetic(Type::Data::UnionTag::Int8));
    static_assert(IsArithmetic(Type::Data::UnionTag::Int16));
    static_assert(IsArithmetic(Type::Data::UnionTag::Int32));
    static_assert(IsArithmetic(Type::Data::UnionTag::Int64));
    static_assert(IsArithmetic(Type::Data::UnionTag::Uint8));
    static_assert(IsArithmetic(Type::Data::UnionTag::Uint16));
    static_assert(IsArithmetic(Type::Data::UnionTag::Uint32));
    static_assert(IsArithmetic(Type::Data::UnionTag::Uint64));
    static_assert(IsArithmetic(Type::Data::UnionTag::Float32));
    static_assert(IsArithmetic(Type::Data::UnionTag::Float64));
    static_assert(!IsArithmetic(Type::Data::UnionTag::Blob));
    static_assert(!IsArithmetic(Type::Data::UnionTag::String));
    static_assert(!IsArithmetic(Type::Data::UnionTag::Array));
    static_assert(!IsArithmetic(Type::Data::UnionTag::List));
    static_assert(!IsArithmetic(Type::Data::UnionTag::Enum));
    static_assert(!IsArithmetic(Type::Data::UnionTag::Struct));
    static_assert(!IsArithmetic(Type::Data::UnionTag::Interface));
    static_assert(!IsArithmetic(Type::Data::UnionTag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsPointer)
{
    static_assert(!IsPointer(Type::Data::UnionTag::Void));
    static_assert(!IsPointer(Type::Data::UnionTag::Bool));
    static_assert(!IsPointer(Type::Data::UnionTag::Int8));
    static_assert(!IsPointer(Type::Data::UnionTag::Int16));
    static_assert(!IsPointer(Type::Data::UnionTag::Int32));
    static_assert(!IsPointer(Type::Data::UnionTag::Int64));
    static_assert(!IsPointer(Type::Data::UnionTag::Uint8));
    static_assert(!IsPointer(Type::Data::UnionTag::Uint16));
    static_assert(!IsPointer(Type::Data::UnionTag::Uint32));
    static_assert(!IsPointer(Type::Data::UnionTag::Uint64));
    static_assert(!IsPointer(Type::Data::UnionTag::Float32));
    static_assert(!IsPointer(Type::Data::UnionTag::Float64));
    static_assert(IsPointer(Type::Data::UnionTag::Blob));
    static_assert(IsPointer(Type::Data::UnionTag::String));
    static_assert(!IsPointer(Type::Data::UnionTag::Array));
    static_assert(IsPointer(Type::Data::UnionTag::List));
    static_assert(!IsPointer(Type::Data::UnionTag::Enum));
    static_assert(IsPointer(Type::Data::UnionTag::Struct));
    static_assert(IsPointer(Type::Data::UnionTag::Interface));
    static_assert(IsPointer(Type::Data::UnionTag::AnyPointer));

    Builder b;
    Type::Builder t = b.AddStruct<Type>();

    // non-array types should be pointers correctly
    const uint16_t max = static_cast<uint16_t>(Type::Data::UnionTag::AnyPointer);
    for (uint16_t i = 0; i < max; ++i)
    {
        const Type::Data::UnionTag tag = Type::Data::UnionTag(i);

        if (tag != Type::Data::UnionTag::Array)
        {
            t.GetData().SetUnionTag(tag);
            HE_EXPECT_EQ(IsPointer(t), IsPointer(tag));
        }
    }

    // Arrays should be pointers if their elements are
    t.GetData().SetUnionTag(Type::Data::UnionTag::Array);
    Type::Data::Array::Builder a = t.GetData().GetArray();
    Type::Builder e = a.InitElementType();
    for (uint16_t i = 0; i < max; ++i)
    {
        const Type::Data::UnionTag tag = Type::Data::UnionTag(i);

        // Arrays of arrays are not supported.
        if (tag == Type::Data::UnionTag::Array)
            continue;

        e.GetData().SetUnionTag(tag);
        HE_EXPECT_EQ(IsPointer(t), IsPointer(tag));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, GetTypeAlign)
{
    Builder b;
    Type::Builder t = b.AddStruct<Type>();

    t.GetData().SetUnionTag(Type::Data::UnionTag::Void); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Bool); HE_EXPECT_EQ(GetTypeAlign(t), 1);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Float32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Float64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Blob); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::String); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    // t.GetData().SetUnionTag(Type::Data::UnionTag::Array); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    t.GetData().SetUnionTag(Type::Data::UnionTag::List); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Enum); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Struct); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Interface); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::AnyPointer); HE_EXPECT_EQ(GetTypeAlign(t), 64);

    t.GetData().SetUnionTag(Type::Data::UnionTag::Array);
    Type::Data::Array::Builder a = t.GetData().GetArray();
    Type::Builder e = a.InitElementType();

    e.GetData().SetUnionTag(Type::Data::UnionTag::Void); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Bool); HE_EXPECT_EQ(GetTypeAlign(t), 1);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Float32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Float64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Blob); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.GetData().SetUnionTag(Type::Data::UnionTag::String); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    // e.GetData().SetUnionTag(Type::Data::UnionTag::Array); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    e.GetData().SetUnionTag(Type::Data::UnionTag::List); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Enum); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Struct); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Interface); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.GetData().SetUnionTag(Type::Data::UnionTag::AnyPointer); HE_EXPECT_EQ(GetTypeAlign(t), 64);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, GetTypeSize)
{
    Builder b;
    Type::Builder t = b.AddStruct<Type>();

    t.GetData().SetUnionTag(Type::Data::UnionTag::Void); HE_EXPECT_EQ(GetTypeSize(t), 0);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Bool); HE_EXPECT_EQ(GetTypeSize(t), 1);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int8); HE_EXPECT_EQ(GetTypeSize(t), 8);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int16); HE_EXPECT_EQ(GetTypeSize(t), 16);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int32); HE_EXPECT_EQ(GetTypeSize(t), 32);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Int64); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint8); HE_EXPECT_EQ(GetTypeSize(t), 8);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint16); HE_EXPECT_EQ(GetTypeSize(t), 16);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint32); HE_EXPECT_EQ(GetTypeSize(t), 32);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Uint64); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Float32); HE_EXPECT_EQ(GetTypeSize(t), 32);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Float64); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Blob); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::String); HE_EXPECT_EQ(GetTypeSize(t), 64);
    // t.GetData().SetUnionTag(Type::Data::UnionTag::Array); HE_EXPECT_EQ(GetTypeSize(t), 0);
    t.GetData().SetUnionTag(Type::Data::UnionTag::List); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Enum); HE_EXPECT_EQ(GetTypeSize(t), 16);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Struct); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::Interface); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.GetData().SetUnionTag(Type::Data::UnionTag::AnyPointer); HE_EXPECT_EQ(GetTypeSize(t), 64);

    t.GetData().SetUnionTag(Type::Data::UnionTag::Array);
    Type::Data::Array::Builder a = t.GetData().GetArray();
    a.SetSize(10);
    Type::Builder e = a.InitElementType();

    e.GetData().SetUnionTag(Type::Data::UnionTag::Void); HE_EXPECT_EQ(GetTypeSize(t), 0);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Bool); HE_EXPECT_EQ(GetTypeSize(t), 10);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int8); HE_EXPECT_EQ(GetTypeSize(t), 80);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int16); HE_EXPECT_EQ(GetTypeSize(t), 160);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int32); HE_EXPECT_EQ(GetTypeSize(t), 320);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Int64); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint8); HE_EXPECT_EQ(GetTypeSize(t), 80);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint16); HE_EXPECT_EQ(GetTypeSize(t), 160);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint32); HE_EXPECT_EQ(GetTypeSize(t), 320);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Uint64); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Float32); HE_EXPECT_EQ(GetTypeSize(t), 320);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Float64); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Blob); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.GetData().SetUnionTag(Type::Data::UnionTag::String); HE_EXPECT_EQ(GetTypeSize(t), 640);
    // e.GetData().SetUnionTag(Type::Data::UnionTag::Array); HE_EXPECT_EQ(GetTypeSize(t), 0);
    e.GetData().SetUnionTag(Type::Data::UnionTag::List); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Enum); HE_EXPECT_EQ(GetTypeSize(t), 160);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Struct); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.GetData().SetUnionTag(Type::Data::UnionTag::Interface); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.GetData().SetUnionTag(Type::Data::UnionTag::AnyPointer); HE_EXPECT_EQ(GetTypeSize(t), 640);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, GeneratedCode)
{
    // TODO
}
