// Copyright Chad Engler

@0x979e892c449bc4d8;

namespace he.schema;

// ------------------------------------------------------------------------------------------------
// Toml Serialization Attributes

struct Toml
{
    enum Compression
    {
        None @0;
        Zstd @1;
    }

    attribute Name(field, enumerator) :String;
    attribute Hex(field) :void;
    attribute Base64(field) :Compression;
    attribute StringLiteral(field) :void;
}

// ------------------------------------------------------------------------------------------------
// Common Utility Structures

struct Uuid
{
    value @0 :uint8[16] $Toml.Hex;
}

struct Vec2f
{
    x @0 :float32;
    y @1 :float32;
}

struct Vec3f
{
    x @0 :float32;
    y @1 :float32;
    z @2 :float32;
}

struct Vec4f
{
    x @0 :float32;
    y @1 :float32;
    z @2 :float32;
    w @3 :float32;
}

struct ScalarRange
{
    data :union
    {
        int :group
        {
            min @0 :int64;
            max @1 :int64;
        }

        uint :group
        {
            min @2 :uint64;
            max @3 :uint64;
        }

        float :group
        {
            min @4 :float64;
            max @5 :float64;
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Schema Structures

struct Brand
{
    struct Scope
    {
        scopeId @0 :uint64;
        params @1 :Type[];
    }

    scopes @0 :Scope[];
}

struct Type
{
    data :union
    {
        void @0 :void;
        bool @1 :void;
        int8 @2 :void;
        int16 @3 :void;
        int32 @4 :void;
        int64 @5 :void;
        uint8 @6 :void;
        uint16 @7 :void;
        uint32 @8 :void;
        uint64 @9 :void;
        float32 @10 :void;
        float64 @11 :void;
        blob @12 :void;
        string @13 :void;
        anyPointer @25 :void;
        anyStruct @26 :void;
        anyList @27 :void;

        array :group
        {
            elementType @14 :Type;
            size @15 :uint16;
        }

        list :group
        {
            elementType @16 :Type;
        }

        enum :group
        {
            id @17 :uint64;
            brand @18 :Brand;
        }

        struct :group
        {
            id @19 :uint64;
            brand @20 :Brand;
        }

        interface :group
        {
            id @21 :uint64;
            brand @22 :Brand;
        }

        parameter :group
        {
            scopeId @23 :uint64;
            index @24 :uint16;
        }
    }
}

struct Value
{
    data :union
    {
        void @0 :void;
        bool @1 :bool;
        int8 @2 :int8;
        int16 @3 :int16;
        int32 @4 :int32;
        int64 @5 :int64;
        uint8 @6 :uint8;
        uint16 @7 :uint16;
        uint32 @8 :uint32;
        uint64 @9 :uint64;
        float32 @10 :float32;
        float64 @11 :float64;
        blob @12 :Blob;
        string @13 :String;
        list @14 :AnyPointer;
        enum @15 :uint16;
        struct @16 :AnyPointer;
    }
}

struct Attribute
{
    id @0 :uint64;
    value @1 :Value;
}

struct Enumerator
{
    name @0 :String;
    declOrder @1 :uint16;   // implicit based on order it was written in the schema file
    ordinal @2 :uint16;     // explicit ordinal value specified using @

    attributes @3 :Attribute[];
}

struct Field
{
    name @0 :String;
    declOrder @1 :uint16;   // implicit based on order it was written in the schema file
    unionTag @2 :uint16;    // tag value for this field if it is in a union

    attributes @3 :Attribute[];

    meta :union
    {
        normal :group
        {
            ordinal @4 :uint16; // explicit ordinal value specified using @
            index @5 :uint16;   // implicit index of the field in its section

            type @6 :Type;
            defaultValue @7 :Value;

            // For data fields this is the offset in units of the field size from the beginning of
            // the data section (after the metadata). For example, for a uint32 field multiply this
            // by 4 to get the byte offset.
            // Always zero for pointer fields.
            dataOffset @8 :uint32;
        }

        group :group
        {
            typeId @9 :uint64; // Id of the type that defines the group's structure
        }

        union :group
        {
            typeId @10 :uint64; // Id of the type that defines the union's structure
        }
    }
}

struct Method
{
    name @0 :String;
    declOrder @1 :uint16;    // implicit based on order it was written in the schema file
    ordinal @2 :uint16;      // explicit ordinal value specified using @

    paramStruct @3 :uint64;
    resultStruct @4 :uint64;

    attributes @5 :Attribute[];
    typeParams @6 :String[];
}

struct SourceInfo
{
    docComment @0 :String;
    file @1 :String;
    line @2 :uint32;
    column @3 :uint32;
}

struct Declaration
{
    name @0 :String;
    id @1 :uint64 $Toml.Hex;
    parentId @2 :uint64 $Toml.Hex;

    source @3 :SourceInfo;

    attributes @4 :Attribute[];
    typeParams @5 :String[];

    children @6 :Declaration[];

    data :union
    {
        file :group
        {
            imports @7 :String[];
        }

        attribute :group
        {
            type @8 :Type;

            targetsAttribute @9 :bool;
            targetsConstant @10 :bool;
            targetsEnum @11 :bool;
            targetsEnumerator @12 :bool;
            targetsField @13 :bool;
            targetsFile @14 :bool;
            targetsInterface @15 :bool;
            targetsMethod @16 :bool;
            targetsParameter @17 :bool;
            targetsStruct @18 :bool;
        }

        constant :group
        {
            type @19 :Type;
            value @20 :Value;
        }

        enum :group
        {
            enumerators @21 :Enumerator[];
        }

        interface :group
        {
            super @22 :Type;
            methods @23 :Method[];
        }

        struct :group
        {
            dataFieldCount @24 :uint16;
            dataWordSize @25 :uint16;
            pointerCount @26 :uint16;
            isGroup @27 :bool;
            isUnion @28 :bool;
            isMethodParams @29 :bool;
            isMethodResults @30 :bool;
            unionTagOffset @31 :uint32;   // offset in the parent struct to union tag, in 2-byte units, if isUnion is true.
            fields @32 :Field[];
        }
    }
}

struct SchemaFile
{
    root @0 :Declaration;
}
