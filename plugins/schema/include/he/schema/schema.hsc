// Copyright Chad Engler

@0x979e892c449bc4d8;

namespace he.schema.tmp;

enum PointerKind
{
    Struct @0;
    List @1;
}

enum ElementSize
{
    Void @0;
    Bit @1;
    Byte @2;
    TwoBytes @3;
    FourBytes @4;
    EightBytes @5;
    Pointer @6;
    Composite @7;
}

struct Type;

struct Brand
{
    struct Scope
    {
        scopeId @0 :uint64;
        params @1 :List<Type>;
    }

    scopes @0 :List<Scope>;
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

        anyPointer :group
        {
            paramScopeId @23 :uint64;
            paramIndex @24 :uint16;
        }
    }
}

struct Value
{
    struct StructValue
    {
        fieldName @0 :String;
        value @1 :Value;
    }

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

        array @14 :List<Value>;
        list @15 :List<Value>;

        enum @16 :uint16;

        struct @17 :List<Value.StructValue>;

        interface @18 :void;

        anyPointer @19 :void;
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

    attributes @3 :List<Attribute>;
}

struct Field
{
    name @0 :String;
    declOrder @1 :uint16;   // implicit based on order it was written in the schema file
    unionTag @2 :uint16;    // tag value for this field if it is in a union

    attributes @3 :List<Attribute>;

    type @4 :Type;
    defaultValue @5 :Value;

    meta :union
    {
        group :group
        {
            typeId @6 :uint64; // Id of the type that defines the group's structure
        }

        union :group
        {
            typeId @7 :uint64; // Id of the type that defines the union's structure
        }

        normal :group
        {
            ordinal @8 :uint16; // explicit ordinal value specified using @
            index @9 :uint16;   // implicit index of the field in its section

            // For data fields this is the offset in units of the field size from the beginning of
            // the data section (after the metadata). For example, for a uint32 field multiply this
            // by 4 to get the byte offset.
            // Always zero for pointer fields.
            dataOffset @10 :uint32;
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

    attributes @5 :List<Attribute>;
    typeParams @6 :List<String>;
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
    id @1 :uint64;
    parentId @2 :uint64;

    source @3 :SourceInfo;

    attributes @4 :List<Attribute>;
    typeParams @5 :List<String>;

    children @6 :List<Declaration>;
    forwards @7 :List<Declaration>;

    data :union
    {
        file @8 :void;

        attribute :group
        {
            type @9 :Type;

            targetsAttribute @10 :bool;
            targetsConst @11 :bool;
            targetsEnum @12 :bool;
            targetsEnumerator @13 :bool;
            targetsField @14 :bool;
            targetsFile @15 :bool;
            targetsInterface @16 :bool;
            targetsMethod @17 :bool;
            targetsParameter @18 :bool;
            targetsStruct @19 :bool;
        }

        const :group
        {
            type @20 :Type;
            value @21 :Value;
        }

        enum :group
        {
            enumerators @22 :List<Enumerator>;
        }

        interface :group
        {
            super @23 :Type;
            methods @24 :List<Method>;
        }

        struct :group
        {
            dataWordSize @25 :uint16;
            pointerCount @26 :uint16;
            isGroup @27 :bool;
            isUnion @28 :bool;
            isAutoGenerated @29 :bool;      // when true this type was generated by the parser, generally for interface method params/result.
            unionTagOffset  @30 :uint32;   // offset to union tag, in 2-byte units, if isUnion is true.
            fields @31 :List<Field>;
        }
    }
}

struct SchemaFile;

struct Import
{
    path @0 :String;
    schema @1 :SchemaFile;
}

struct SchemaFile
{
    root @0 :Declaration;
    imports @1 :List<Import>;
    attributes @2 :List<Attribute>;
}
