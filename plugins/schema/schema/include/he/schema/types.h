// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"

namespace he::schema
{
    /// A 64-bit identifier for a type in the schema.
    using TypeId = uint64_t;

    /// A word in schema is always 64-bits.
    using Word = uint64_t;

    /// A flag used to mark a value as a type ID. The MSB of IDs is always set.
    ///
    /// Since the `@` symbol is used for field ordinals and type IDs it can be easy to mix
    /// them up. Someone may attempt to write an ID for a type declaration manually (like @2)
    /// which would be problematic since IDs are intended to be globally unique. Setting the
    /// MSB of IDs allows us to quickly spot ones that are incorrect in code, as well as in
    /// the schema language. Because the top bit is always set IDs are always printed as
    /// 32 hex digits without any leading zeroes.
    constexpr TypeId TypeIdFlag = (1ull << 63);

    /// A structer that "hashes" type IDs by just returning the ID itself. Type IDs are already
    /// hashes so they don't need to mix any bits to reach uniform distribution.
    struct TypeIdHasher
    {
        size_t operator()(TypeId id) const { return static_cast<size_t>(id); }
    };

    /// Helper type to represent the `void` schema type in C++. All instances of Void are
    /// empty and considered equivalent.
    struct Void
    {
        constexpr bool operator==(Void) const { return true; }
        constexpr bool operator!=(Void) const { return false; }
    };

    /// Enumeration of the kinds of pointers that exists in the schema binary format.
    ///
    /// @note The numeric values of these enums are used in persisted binary data, changing
    /// the values can create backwards incompatibilities.
    enum class PointerKind : uint16_t
    {
        Struct = 0,
        List = 1,

        _Count,
    };

    /// Enumeration of the element sizes that exist in the schema binary format.
    ///
    /// @note The numeric values of these enums are used in persisted binary data, changing
    /// the values can create backwards incompatibilities.
    enum class ElementSize : uint16_t
    {
        Void = 0,
        Bit = 1,
        Byte = 2,
        TwoBytes = 3,
        FourBytes = 4,
        EightBytes = 5,
        Pointer = 6,
        Composite = 7,

        _Count,
    };

    /// Enumeration of the kinds of declarations that have generated code.
    enum class DeclKind : uint16_t
    {
        Attribute,
        Constant,
        Enum,
        File,
        Interface,
        Struct,

        _Count,
    };

    /// The FieldInfo structure stores information about a schema struct field in generated code.
    struct FieldInfo
    {
        /// Offset of the field in the C++ structure.
        const uint32_t nativeOffset;

        /// Pointer to the raw schema words for this field.
        const Word* const schema;
    };

    /// The DeclInfo structure stores information about a schema declaration in generated code.
    struct DeclInfo
    {
        /// The unique ID of the declaration.
        const TypeId id;

        /// The unique ID of the parent of the declaration.
        const TypeId parentId;

        /// The kind of delcaration this is.
        const DeclKind kind;

        /// Number of data fields in the struct. Always zero when `kind != DeclKind::Struct`.
        const uint16_t dataFieldCount;

        /// Size of the struct's data section in words. Always zero when `kind != DeclKind::Struct`.
        const uint16_t dataWordSize;

        /// Number of pointers in the struct. Always zero when `kind != DeclKind::Struct`.
        const uint16_t pointerCount;

        /// Pointer to the raw schema words for this declaration.
        const Word* const schema;

        /// Array of pointers to types that are used by this declaration, in order of their IDs.
        const DeclInfo* const* const dependencies;

        /// Number of dependencies in the `dependencies` array.
        const uint32_t dependencyCount;

        /// Array of field info structures.
        const FieldInfo* fields;

        /// Number of field info structures in the `fields` array.
        const uint32_t fieldCount;
    };

    template <TypeId Id>
    struct DeclInfoForId;

    template <Enum T>
    struct EnumInfo;

    #define HE_SCHEMA_DECL_INFO_FOR_ID(id) \
        template <> struct DeclInfoForId<id> { static const DeclInfo Value; }

    #define HE_SCHEMA_DECL_(id, parentId, kind) \
        static constexpr ::he::schema::TypeId Id = id; \
        static constexpr ::he::schema::TypeId ParentId = parentId; \
        static constexpr ::he::schema::DeclKind Kind = ::he::schema::DeclKind::kind; \
        static constexpr const ::he::schema::DeclInfo& DeclInfo = ::he::schema::DeclInfoForId<id>::Value

    #define HE_SCHEMA_DECL_ENUM(type, id, parentId) \
        template <> struct EnumInfo<type> { HE_SCHEMA_DECL_(id, parentId, Enum); };

    #define HE_SCHEMA_DECL_ATTRIBUTE(id, parentId) \
        HE_SCHEMA_DECL_(id, parentId, Attribute)

    #define HE_SCHEMA_DECL_INTERFACE(id, parentId) \
        HE_SCHEMA_DECL_(id, parentId, Interface)

    #define HE_SCHEMA_DECL_STRUCT(id, parentId, dataFieldCount, dataWordSize, pointerCount) \
        HE_SCHEMA_DECL_(id, parentId, Struct); \
        static constexpr uint16_t DataFieldCount = dataFieldCount; \
        static constexpr uint16_t DataWordSize = dataWordSize; \
        static constexpr uint16_t PointerCount = pointerCount
}
