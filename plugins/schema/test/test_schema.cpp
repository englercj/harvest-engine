// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/allocator.h"
#include "he/core/test.h"

using namespace he;
using namespace he::schema;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, BaseType, Constants)
{
    static_assert(InvalidSchemaIndex == 0xffff);
    HE_EXPECT_EQ(InvalidSchemaIndex, 0xffff);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, BaseType, IsIntegral)
{
    HE_EXPECT(!IsIntegral(BaseType::Unknown));
    HE_EXPECT(IsIntegral(BaseType::Bool));
    HE_EXPECT(IsIntegral(BaseType::Int8));
    HE_EXPECT(IsIntegral(BaseType::Int16));
    HE_EXPECT(IsIntegral(BaseType::Int32));
    HE_EXPECT(IsIntegral(BaseType::Int64));
    HE_EXPECT(IsIntegral(BaseType::Uint8));
    HE_EXPECT(IsIntegral(BaseType::Uint16));
    HE_EXPECT(IsIntegral(BaseType::Uint32));
    HE_EXPECT(IsIntegral(BaseType::Uint64));
    HE_EXPECT(!IsIntegral(BaseType::Float32));
    HE_EXPECT(!IsIntegral(BaseType::Float64));
    HE_EXPECT(!IsIntegral(BaseType::Array));
    HE_EXPECT(!IsIntegral(BaseType::List));
    HE_EXPECT(!IsIntegral(BaseType::Map));
    HE_EXPECT(!IsIntegral(BaseType::Set));
    HE_EXPECT(!IsIntegral(BaseType::String));
    HE_EXPECT(!IsIntegral(BaseType::Vector));
    HE_EXPECT(!IsIntegral(BaseType::Enum));
    HE_EXPECT(!IsIntegral(BaseType::Interface));
    HE_EXPECT(!IsIntegral(BaseType::Struct));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, BaseType, IsFloat)
{
    HE_EXPECT(!IsFloat(BaseType::Unknown));
    HE_EXPECT(!IsFloat(BaseType::Bool));
    HE_EXPECT(!IsFloat(BaseType::Int8));
    HE_EXPECT(!IsFloat(BaseType::Int16));
    HE_EXPECT(!IsFloat(BaseType::Int32));
    HE_EXPECT(!IsFloat(BaseType::Int64));
    HE_EXPECT(!IsFloat(BaseType::Uint8));
    HE_EXPECT(!IsFloat(BaseType::Uint16));
    HE_EXPECT(!IsFloat(BaseType::Uint32));
    HE_EXPECT(!IsFloat(BaseType::Uint64));
    HE_EXPECT(IsFloat(BaseType::Float32));
    HE_EXPECT(IsFloat(BaseType::Float64));
    HE_EXPECT(!IsFloat(BaseType::Array));
    HE_EXPECT(!IsFloat(BaseType::List));
    HE_EXPECT(!IsFloat(BaseType::Map));
    HE_EXPECT(!IsFloat(BaseType::Set));
    HE_EXPECT(!IsFloat(BaseType::String));
    HE_EXPECT(!IsFloat(BaseType::Vector));
    HE_EXPECT(!IsFloat(BaseType::Enum));
    HE_EXPECT(!IsFloat(BaseType::Interface));
    HE_EXPECT(!IsFloat(BaseType::Struct));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, BaseType, IsArithmetic)
{
    HE_EXPECT(!IsArithmetic(BaseType::Unknown));
    HE_EXPECT(IsArithmetic(BaseType::Bool));
    HE_EXPECT(IsArithmetic(BaseType::Int8));
    HE_EXPECT(IsArithmetic(BaseType::Int16));
    HE_EXPECT(IsArithmetic(BaseType::Int32));
    HE_EXPECT(IsArithmetic(BaseType::Int64));
    HE_EXPECT(IsArithmetic(BaseType::Uint8));
    HE_EXPECT(IsArithmetic(BaseType::Uint16));
    HE_EXPECT(IsArithmetic(BaseType::Uint32));
    HE_EXPECT(IsArithmetic(BaseType::Uint64));
    HE_EXPECT(IsArithmetic(BaseType::Float32));
    HE_EXPECT(IsArithmetic(BaseType::Float64));
    HE_EXPECT(!IsArithmetic(BaseType::Array));
    HE_EXPECT(!IsArithmetic(BaseType::List));
    HE_EXPECT(!IsArithmetic(BaseType::Map));
    HE_EXPECT(!IsArithmetic(BaseType::Set));
    HE_EXPECT(!IsArithmetic(BaseType::String));
    HE_EXPECT(!IsArithmetic(BaseType::Vector));
    HE_EXPECT(!IsArithmetic(BaseType::Enum));
    HE_EXPECT(!IsArithmetic(BaseType::Interface));
    HE_EXPECT(!IsArithmetic(BaseType::Struct));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, BaseType, IsObject)
{
    HE_EXPECT(!IsObject(BaseType::Unknown));
    HE_EXPECT(!IsObject(BaseType::Bool));
    HE_EXPECT(!IsObject(BaseType::Int8));
    HE_EXPECT(!IsObject(BaseType::Int16));
    HE_EXPECT(!IsObject(BaseType::Int32));
    HE_EXPECT(!IsObject(BaseType::Int64));
    HE_EXPECT(!IsObject(BaseType::Uint8));
    HE_EXPECT(!IsObject(BaseType::Uint16));
    HE_EXPECT(!IsObject(BaseType::Uint32));
    HE_EXPECT(!IsObject(BaseType::Uint64));
    HE_EXPECT(!IsObject(BaseType::Float32));
    HE_EXPECT(!IsObject(BaseType::Float64));
    HE_EXPECT(IsObject(BaseType::Array));
    HE_EXPECT(IsObject(BaseType::List));
    HE_EXPECT(IsObject(BaseType::Map));
    HE_EXPECT(IsObject(BaseType::Set));
    HE_EXPECT(IsObject(BaseType::String));
    HE_EXPECT(IsObject(BaseType::Vector));
    HE_EXPECT(!IsObject(BaseType::Enum));
    HE_EXPECT(IsObject(BaseType::Interface));
    HE_EXPECT(IsObject(BaseType::Struct));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, SchemaDef, MarkTypeUsed)
{
    SchemaDef schema(CrtAllocator::Get());

    HE_EXPECT_EQ(schema.usedTypes, 0);
    HE_EXPECT(!schema.IsTypeUsed(BaseType::String));

    schema.MarkTypeUsed(BaseType::String);
    HE_EXPECT_EQ(schema.usedTypes, static_cast<uint64_t>(BaseType::String));
    HE_EXPECT(schema.IsTypeUsed(BaseType::String));

    schema.MarkTypeUsed(BaseType::Int64);
    HE_EXPECT_EQ(schema.usedTypes, static_cast<uint64_t>(BaseType::String) | static_cast<uint64_t>(BaseType::Int64));
    HE_EXPECT(schema.IsTypeUsed(BaseType::String));
    HE_EXPECT(schema.IsTypeUsed(BaseType::Int64));
}
