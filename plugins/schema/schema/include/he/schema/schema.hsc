// Copyright Chad Engler

@0x979e892c449bc4d8;

namespace he.schema;

// ------------------------------------------------------------------------------------------------
// KDL Serialization Attributes

struct KDL
{
    // A type of compression to be applied to Base64 fields.
    enum Compression
    {
        // Applies no compression
        None @0;

        // Applies ZStandard compression
        Zstd @1;
    }

    // Specifies a name that can be used as the node identifier for this field in KDL documents.
    // This is useful to keep backwards compatibility with existing documents after a name change.
    // When serializing to a KDL document the field's name is used as the node identifier.
    // If you want to use a specific name while serializing to KDL use `$KDL.Name("")` instead.
    //
    // Example:
    // ```hsc
    // struct Author
    // {
    //     name: String;
    //     email: String;
    // }
    // struct Plugin
    // {
    //     authors: Author[] $KDL.Alias("author");
    // }
    // ```
    // A valid `Plugin` KDL document is:
    // ```kdl
    // author { name "Name Value 1"; email "Email Value 1"; }
    // author { name "Name Value 2"; email "Email Value 2"; }
    // authors { name "Name Value 3"; email "Email Value 3"; }
    // authors { name "Name Value 4"; email "Email Value 4"; }
    // ```
    attribute Alias(field) :String;

    // For fields, this specifies the name to use use as the node identifier in KDL documents.
    // For enumerators, this specifies the string to use for that value in KDL documents.
    //
    // Example:
    // ```hsc
    // struct Module
    // {
    //     name: String $KDL.Name("n");
    //     type: String $KDL.Name("t");
    // }
    // ```
    // A valid `Module` KDL document is:
    // ```kdl
    // n "module1"
    // t "type 1"
    // ```
    attribute Name(field, enumerator) :String;

    // Specifies the name of a property whose value is used as the node identifier for this field,
    // or the node identifier for elements of an array/list field, in KDL documents.
    // Only considered for fields with a struct type, or an array/list field that has a struct
    // element type.
    //
    // By default array/list fields with a struct element type will use "-" as the node identifier
    // for all elements.
    //
    // Example:
    // ```hsc
    // struct Module
    // {
    //     name: String;
    //     type: String;
    // }
    // struct Plugin
    // {
    //     unnamed: Module[];
    //     named: Module[] $KDL.NameProperty("name");
    // }
    // ```
    // A valid `Plugin` KDL document is:
    // ```kdl
    // unnamed {
    //     - { name "module1"; type "type 1"; }
    //     - { name "module2"; type "type 2"; }
    // }
    // named {
    //     module1 { type "type 1"; }
    //     module2 { type "type 2"; }
    // }
    // ```
    attribute NameProperty(field) :String

    // Formats the struct or field as an inline node in KDL documents. A property name may
    // optionally be specified which will treat that property as the node's value.
    // Only considered on structs, fields with a struct type, or array/list fields with a struct
    // element type.
    //
    // Example:
    // ```hsc
    // struct Module
    // {
    //     name: String;
    //     type: String;
    // }
    // struct Author $KDL.Inline("name")
    // {
    //     name: String;
    //     email: String;
    // }
    // struct Plugin
    // {
    //     author: Author;
    //     moduleInline: Module $KDL.Inline;
    //     module: Module;
    // }
    // ```
    // A valid `Plugin` KDL document is:
    // ```kdl
    // author "Author Name" email="Author Email"
    // module_inline name="module_name" type="static"
    // module {
    //     name "module_name"
    //     type "static"
    // }
    // ```
    attribute Inline(struct, field) :String;

    // Formats Blobs, Lists of uint8, and Arrays of uint8 as base64 strings. The compression
    // method is applied to the data before being encoded into base64. Base64 encoding without
    // any compression is the default behavior for Blobs, Lists of uint8, and Arrays of uint8.
    attribute Base64(field) :Compression;

    // Formats integral values using hex representation (`0xabcdef`)
    // Also will format Blobs, Lists of uint8, and Arrays of uint8 as strings of hex characters.
    // Base64 encoding without any compression is the default behavior for Blobs, Lists of uint8,
    // and Arrays of uint8.
    attribute Hex(field) :void;

    // Formats integral values using octal representation (`0o755`).
    attribute Octal(field) :void;

    // Formats integral values using binary representation (`0b010101`)
    attribute Binary(field) :void;

    // Formats floating point values using general representation. This is the default behavior.
    // The value is the precision to use when serializing, this is optional.
    attribute General(field) :int32;

    // Formats floating point values using fixed representation (`0.123`).
    // The value is the precision to use when serializing, this is optional.
    attribute Fixed(field) :int32;

    // Formats floating point values using fixed representation (`123e-3`).
    // The value is the precision to use when serializing, this is optional.
    attribute Exponent(field) :int32;
}

// ------------------------------------------------------------------------------------------------
// Toml Serialization Attributes

struct Toml
{
    enum Compression
    {
        // Applies no compression
        None @0;

        // Applies ZStandard compression
        Zstd @1;
    }

    // Overrides the name used for this field when serializing to and from TOML.
    attribute Name(field, enumerator) :String;

    // Formats Blobs, Lists of uint8, and Arrays of uint8 as base64 strings. The compression
    // method is applied to the data before being encoded into base64. Base64 encoding without
    // any compression is the default behavior for Blobs, Lists of uint8, and Arrays of uint8.
    attribute Base64(field) :Compression;

    // Formats unsigned integral values using decimal representation (`12345`).
    // This is the default behavior.
    attribute Decimal(field) :void;

    // Formats unsigned integral values using hex representation (`0xabcdef`)
    // Also will format Blobs, Lists of uint8, and Arrays of uint8 as strings of hex characters.
    // Base64 encoding without any compression is the default behavior for Blobs, Lists of uint8,
    // and Arrays of uint8.
    attribute Hex(field) :void;

    // Formats unsigned integral values using octal representation (`0o755`).
    attribute Octal(field) :void;

    // Formats unsigned integral values using binary representation (`0b010101`)
    attribute Binary(field) :void;

    // Formats floating point values using general representation. This is the default behavior.
    attribute General(field) :void;

    // Formats floating point values using fixed representation (`0.123`).
    attribute Fixed(field) :void;

    // Formats floating point values using fixed representation (`123e-3`).
    attribute Exponent(field) :void;

    // Specified the precision of floating point values when serializing.
    attribute Precision(field) :int32;

    // Formats strings as basic strings (`"abc"`). This is the default behavior for strings.
    attribute Basic(field) :void;

    // Formats strings as literal strings (`'abc'`).
    attribute Literal(field) :void;
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
        scopeId @0 :uint64 $Toml.Hex;
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
            id @17 :uint64 $Toml.Hex;
            brand @18 :Brand;
        }

        struct :group
        {
            id @19 :uint64 $Toml.Hex;
            brand @20 :Brand;
        }

        interface :group
        {
            id @21 :uint64 $Toml.Hex;
            brand @22 :Brand;
        }

        parameter :group
        {
            scopeId @23 :uint64 $Toml.Hex;
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
    id @0 :uint64 $Toml.Hex;
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
            typeId @9 :uint64 $Toml.Hex; // Id of the type that defines the group's structure
        }

        union :group
        {
            typeId @10 :uint64 $Toml.Hex; // Id of the type that defines the union's structure
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
