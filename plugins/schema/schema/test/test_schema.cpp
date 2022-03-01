// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/allocator.h"
#include "he/core/test.h"

using namespace he;
using namespace he::schema;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsSignedIntegral)
{
    static_assert(!IsSignedIntegral(Type::Data::Tag::Void));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Bool));
    static_assert(IsSignedIntegral(Type::Data::Tag::Int8));
    static_assert(IsSignedIntegral(Type::Data::Tag::Int16));
    static_assert(IsSignedIntegral(Type::Data::Tag::Int32));
    static_assert(IsSignedIntegral(Type::Data::Tag::Int64));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Uint8));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Uint16));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Uint32));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Uint64));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Float32));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Float64));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Blob));
    static_assert(!IsSignedIntegral(Type::Data::Tag::String));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Array));
    static_assert(!IsSignedIntegral(Type::Data::Tag::List));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Enum));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Struct));
    static_assert(!IsSignedIntegral(Type::Data::Tag::Interface));
    static_assert(!IsSignedIntegral(Type::Data::Tag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsUnsignedIntegral)
{
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Void));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Bool));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Int8));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Int16));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Int32));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Int64));
    static_assert(IsUnsignedIntegral(Type::Data::Tag::Uint8));
    static_assert(IsUnsignedIntegral(Type::Data::Tag::Uint16));
    static_assert(IsUnsignedIntegral(Type::Data::Tag::Uint32));
    static_assert(IsUnsignedIntegral(Type::Data::Tag::Uint64));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Float32));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Float64));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Blob));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::String));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Array));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::List));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Enum));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Struct));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::Interface));
    static_assert(!IsUnsignedIntegral(Type::Data::Tag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsIntegral)
{
    static_assert(!IsIntegral(Type::Data::Tag::Void));
    static_assert(!IsIntegral(Type::Data::Tag::Bool));
    static_assert(IsIntegral(Type::Data::Tag::Int8));
    static_assert(IsIntegral(Type::Data::Tag::Int16));
    static_assert(IsIntegral(Type::Data::Tag::Int32));
    static_assert(IsIntegral(Type::Data::Tag::Int64));
    static_assert(IsIntegral(Type::Data::Tag::Uint8));
    static_assert(IsIntegral(Type::Data::Tag::Uint16));
    static_assert(IsIntegral(Type::Data::Tag::Uint32));
    static_assert(IsIntegral(Type::Data::Tag::Uint64));
    static_assert(!IsIntegral(Type::Data::Tag::Float32));
    static_assert(!IsIntegral(Type::Data::Tag::Float64));
    static_assert(!IsIntegral(Type::Data::Tag::Blob));
    static_assert(!IsIntegral(Type::Data::Tag::String));
    static_assert(!IsIntegral(Type::Data::Tag::Array));
    static_assert(!IsIntegral(Type::Data::Tag::List));
    static_assert(!IsIntegral(Type::Data::Tag::Enum));
    static_assert(!IsIntegral(Type::Data::Tag::Struct));
    static_assert(!IsIntegral(Type::Data::Tag::Interface));
    static_assert(!IsIntegral(Type::Data::Tag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsFloat)
{
    static_assert(!IsFloat(Type::Data::Tag::Void));
    static_assert(!IsFloat(Type::Data::Tag::Bool));
    static_assert(!IsFloat(Type::Data::Tag::Int8));
    static_assert(!IsFloat(Type::Data::Tag::Int16));
    static_assert(!IsFloat(Type::Data::Tag::Int32));
    static_assert(!IsFloat(Type::Data::Tag::Int64));
    static_assert(!IsFloat(Type::Data::Tag::Uint8));
    static_assert(!IsFloat(Type::Data::Tag::Uint16));
    static_assert(!IsFloat(Type::Data::Tag::Uint32));
    static_assert(!IsFloat(Type::Data::Tag::Uint64));
    static_assert(IsFloat(Type::Data::Tag::Float32));
    static_assert(IsFloat(Type::Data::Tag::Float64));
    static_assert(!IsFloat(Type::Data::Tag::Blob));
    static_assert(!IsFloat(Type::Data::Tag::String));
    static_assert(!IsFloat(Type::Data::Tag::Array));
    static_assert(!IsFloat(Type::Data::Tag::List));
    static_assert(!IsFloat(Type::Data::Tag::Enum));
    static_assert(!IsFloat(Type::Data::Tag::Struct));
    static_assert(!IsFloat(Type::Data::Tag::Interface));
    static_assert(!IsFloat(Type::Data::Tag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsArithmetic)
{
    static_assert(!IsArithmetic(Type::Data::Tag::Void));
    static_assert(!IsArithmetic(Type::Data::Tag::Bool));
    static_assert(IsArithmetic(Type::Data::Tag::Int8));
    static_assert(IsArithmetic(Type::Data::Tag::Int16));
    static_assert(IsArithmetic(Type::Data::Tag::Int32));
    static_assert(IsArithmetic(Type::Data::Tag::Int64));
    static_assert(IsArithmetic(Type::Data::Tag::Uint8));
    static_assert(IsArithmetic(Type::Data::Tag::Uint16));
    static_assert(IsArithmetic(Type::Data::Tag::Uint32));
    static_assert(IsArithmetic(Type::Data::Tag::Uint64));
    static_assert(IsArithmetic(Type::Data::Tag::Float32));
    static_assert(IsArithmetic(Type::Data::Tag::Float64));
    static_assert(!IsArithmetic(Type::Data::Tag::Blob));
    static_assert(!IsArithmetic(Type::Data::Tag::String));
    static_assert(!IsArithmetic(Type::Data::Tag::Array));
    static_assert(!IsArithmetic(Type::Data::Tag::List));
    static_assert(!IsArithmetic(Type::Data::Tag::Enum));
    static_assert(!IsArithmetic(Type::Data::Tag::Struct));
    static_assert(!IsArithmetic(Type::Data::Tag::Interface));
    static_assert(!IsArithmetic(Type::Data::Tag::AnyPointer));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, IsPointer)
{
    static_assert(!IsPointer(Type::Data::Tag::Void));
    static_assert(!IsPointer(Type::Data::Tag::Bool));
    static_assert(!IsPointer(Type::Data::Tag::Int8));
    static_assert(!IsPointer(Type::Data::Tag::Int16));
    static_assert(!IsPointer(Type::Data::Tag::Int32));
    static_assert(!IsPointer(Type::Data::Tag::Int64));
    static_assert(!IsPointer(Type::Data::Tag::Uint8));
    static_assert(!IsPointer(Type::Data::Tag::Uint16));
    static_assert(!IsPointer(Type::Data::Tag::Uint32));
    static_assert(!IsPointer(Type::Data::Tag::Uint64));
    static_assert(!IsPointer(Type::Data::Tag::Float32));
    static_assert(!IsPointer(Type::Data::Tag::Float64));
    static_assert(IsPointer(Type::Data::Tag::Blob));
    static_assert(IsPointer(Type::Data::Tag::String));
    static_assert(!IsPointer(Type::Data::Tag::Array));
    static_assert(IsPointer(Type::Data::Tag::List));
    static_assert(!IsPointer(Type::Data::Tag::Enum));
    static_assert(IsPointer(Type::Data::Tag::Struct));
    static_assert(IsPointer(Type::Data::Tag::Interface));
    static_assert(IsPointer(Type::Data::Tag::AnyPointer));

    Builder b;
    Type::Builder t = b.AddStruct<Type>();

    // non-array types should be pointers correctly
    const uint16_t max = static_cast<uint16_t>(Type::Data::Tag::AnyPointer);
    for (uint16_t i = 0; i < max; ++i)
    {
        const Type::Data::Tag tag = Type::Data::Tag(i);

        if (tag != Type::Data::Tag::Array)
        {
            t.Data().SetTag(tag);
            HE_EXPECT_EQ(IsPointer(t), IsPointer(tag));
        }
    }

    // Arrays should be pointers if their elements are
    t.Data().SetTag(Type::Data::Tag::Array);
    Type::Data::Array::Builder a = t.Data().Array();
    Type::Builder e = a.InitElementType();
    for (uint16_t i = 0; i < max; ++i)
    {
        const Type::Data::Tag tag = Type::Data::Tag(i);
        e.Data().SetTag(tag);
        HE_EXPECT_EQ(IsPointer(t), IsPointer(tag));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, GetTypeAlign)
{
    Builder b;
    Type::Builder t = b.AddStruct<Type>();

    t.Data().SetTag(Type::Data::Tag::Void); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    t.Data().SetTag(Type::Data::Tag::Bool); HE_EXPECT_EQ(GetTypeAlign(t), 1);
    t.Data().SetTag(Type::Data::Tag::Int8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    t.Data().SetTag(Type::Data::Tag::Int16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    t.Data().SetTag(Type::Data::Tag::Int32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    t.Data().SetTag(Type::Data::Tag::Int64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.Data().SetTag(Type::Data::Tag::Uint8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    t.Data().SetTag(Type::Data::Tag::Uint16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    t.Data().SetTag(Type::Data::Tag::Uint32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    t.Data().SetTag(Type::Data::Tag::Uint64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.Data().SetTag(Type::Data::Tag::Float32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    t.Data().SetTag(Type::Data::Tag::Float64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.Data().SetTag(Type::Data::Tag::Blob); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.Data().SetTag(Type::Data::Tag::String); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    // t.Data().SetTag(Type::Data::Tag::Array); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    t.Data().SetTag(Type::Data::Tag::List); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.Data().SetTag(Type::Data::Tag::Enum); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    t.Data().SetTag(Type::Data::Tag::Struct); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.Data().SetTag(Type::Data::Tag::Interface); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    t.Data().SetTag(Type::Data::Tag::AnyPointer); HE_EXPECT_EQ(GetTypeAlign(t), 64);

    t.Data().SetTag(Type::Data::Tag::Array);
    Type::Data::Array::Builder a = t.Data().Array();
    Type::Builder e = a.InitElementType();

    e.Data().SetTag(Type::Data::Tag::Void); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    e.Data().SetTag(Type::Data::Tag::Bool); HE_EXPECT_EQ(GetTypeAlign(t), 1);
    e.Data().SetTag(Type::Data::Tag::Int8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    e.Data().SetTag(Type::Data::Tag::Int16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    e.Data().SetTag(Type::Data::Tag::Int32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    e.Data().SetTag(Type::Data::Tag::Int64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.Data().SetTag(Type::Data::Tag::Uint8); HE_EXPECT_EQ(GetTypeAlign(t), 8);
    e.Data().SetTag(Type::Data::Tag::Uint16); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    e.Data().SetTag(Type::Data::Tag::Uint32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    e.Data().SetTag(Type::Data::Tag::Uint64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.Data().SetTag(Type::Data::Tag::Float32); HE_EXPECT_EQ(GetTypeAlign(t), 32);
    e.Data().SetTag(Type::Data::Tag::Float64); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.Data().SetTag(Type::Data::Tag::Blob); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.Data().SetTag(Type::Data::Tag::String); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    // e.Data().SetTag(Type::Data::Tag::Array); HE_EXPECT_EQ(GetTypeAlign(t), 0);
    e.Data().SetTag(Type::Data::Tag::List); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.Data().SetTag(Type::Data::Tag::Enum); HE_EXPECT_EQ(GetTypeAlign(t), 16);
    e.Data().SetTag(Type::Data::Tag::Struct); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.Data().SetTag(Type::Data::Tag::Interface); HE_EXPECT_EQ(GetTypeAlign(t), 64);
    e.Data().SetTag(Type::Data::Tag::AnyPointer); HE_EXPECT_EQ(GetTypeAlign(t), 64);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, GetTypeSize)
{
    Builder b;
    Type::Builder t = b.AddStruct<Type>();

    t.Data().SetTag(Type::Data::Tag::Void); HE_EXPECT_EQ(GetTypeSize(t), 0);
    t.Data().SetTag(Type::Data::Tag::Bool); HE_EXPECT_EQ(GetTypeSize(t), 1);
    t.Data().SetTag(Type::Data::Tag::Int8); HE_EXPECT_EQ(GetTypeSize(t), 8);
    t.Data().SetTag(Type::Data::Tag::Int16); HE_EXPECT_EQ(GetTypeSize(t), 16);
    t.Data().SetTag(Type::Data::Tag::Int32); HE_EXPECT_EQ(GetTypeSize(t), 32);
    t.Data().SetTag(Type::Data::Tag::Int64); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.Data().SetTag(Type::Data::Tag::Uint8); HE_EXPECT_EQ(GetTypeSize(t), 8);
    t.Data().SetTag(Type::Data::Tag::Uint16); HE_EXPECT_EQ(GetTypeSize(t), 16);
    t.Data().SetTag(Type::Data::Tag::Uint32); HE_EXPECT_EQ(GetTypeSize(t), 32);
    t.Data().SetTag(Type::Data::Tag::Uint64); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.Data().SetTag(Type::Data::Tag::Float32); HE_EXPECT_EQ(GetTypeSize(t), 32);
    t.Data().SetTag(Type::Data::Tag::Float64); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.Data().SetTag(Type::Data::Tag::Blob); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.Data().SetTag(Type::Data::Tag::String); HE_EXPECT_EQ(GetTypeSize(t), 64);
    // t.Data().SetTag(Type::Data::Tag::Array); HE_EXPECT_EQ(GetTypeSize(t), 0);
    t.Data().SetTag(Type::Data::Tag::List); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.Data().SetTag(Type::Data::Tag::Enum); HE_EXPECT_EQ(GetTypeSize(t), 16);
    t.Data().SetTag(Type::Data::Tag::Struct); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.Data().SetTag(Type::Data::Tag::Interface); HE_EXPECT_EQ(GetTypeSize(t), 64);
    t.Data().SetTag(Type::Data::Tag::AnyPointer); HE_EXPECT_EQ(GetTypeSize(t), 64);

    t.Data().SetTag(Type::Data::Tag::Array);
    Type::Data::Array::Builder a = t.Data().Array();
    a.SetSize(10);
    Type::Builder e = a.InitElementType();

    e.Data().SetTag(Type::Data::Tag::Void); HE_EXPECT_EQ(GetTypeSize(t), 0);
    e.Data().SetTag(Type::Data::Tag::Bool); HE_EXPECT_EQ(GetTypeSize(t), 10);
    e.Data().SetTag(Type::Data::Tag::Int8); HE_EXPECT_EQ(GetTypeSize(t), 80);
    e.Data().SetTag(Type::Data::Tag::Int16); HE_EXPECT_EQ(GetTypeSize(t), 160);
    e.Data().SetTag(Type::Data::Tag::Int32); HE_EXPECT_EQ(GetTypeSize(t), 320);
    e.Data().SetTag(Type::Data::Tag::Int64); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.Data().SetTag(Type::Data::Tag::Uint8); HE_EXPECT_EQ(GetTypeSize(t), 80);
    e.Data().SetTag(Type::Data::Tag::Uint16); HE_EXPECT_EQ(GetTypeSize(t), 160);
    e.Data().SetTag(Type::Data::Tag::Uint32); HE_EXPECT_EQ(GetTypeSize(t), 320);
    e.Data().SetTag(Type::Data::Tag::Uint64); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.Data().SetTag(Type::Data::Tag::Float32); HE_EXPECT_EQ(GetTypeSize(t), 320);
    e.Data().SetTag(Type::Data::Tag::Float64); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.Data().SetTag(Type::Data::Tag::Blob); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.Data().SetTag(Type::Data::Tag::String); HE_EXPECT_EQ(GetTypeSize(t), 640);
    // e.Data().SetTag(Type::Data::Tag::Array); HE_EXPECT_EQ(GetTypeSize(t), 0);
    e.Data().SetTag(Type::Data::Tag::List); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.Data().SetTag(Type::Data::Tag::Enum); HE_EXPECT_EQ(GetTypeSize(t), 160);
    e.Data().SetTag(Type::Data::Tag::Struct); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.Data().SetTag(Type::Data::Tag::Interface); HE_EXPECT_EQ(GetTypeSize(t), 640);
    e.Data().SetTag(Type::Data::Tag::AnyPointer); HE_EXPECT_EQ(GetTypeSize(t), 640);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, schema, GeneratedCode)
{
    // TODO
}
