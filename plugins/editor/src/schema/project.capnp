# Copyright Chad Engler

@0x979e892c449bc4d8;

using Cxx = import "/capnp/c++.capnp";
using Json = import "/capnp/compat/json.capnp";

$Cxx.namespace("he::editor::schema");

struct Brand
{
    struct Scope
    {
        scopeId @0 :UInt64;
        params @1 :List(Type);
    }

    scopes @0 :List(Scope);
}

struct Type
{
    data :union
    {
        void @0 :Void;
        bool @1 :Void;
        int8 @2 :Void;
        int16 @3 :Void;
        int32 @4 :Void;
        int64 @5 :Void;
        uint8 @6 :Void;
        uint16 @7 :Void;
        uint32 @8 :Void;
        uint64 @9 :Void;
        float32 @10 :Void;
        float64 @11 :Void;
        blob @12 :Void;
        string @13 :Void;

        array :group
        {
            elementType @14 :Type;
            size @15 :UInt16;
        }

        list :group
        {
            elementType @16 :Type;
        }

        enum :group
        {
            id @17 :UInt64;
            brand @18 :Brand;
        }

        struct :group
        {
            id @19 :UInt64;
            brand @20 :Brand;
        }

        interface :group
        {
            id @21 :UInt64;
            brand @22 :Brand;
        }

        anyPointer :group
        {
            paramScopeId @23 :UInt64;
            paramIndex @24 :UInt16;
        }
    }
}

struct Value
{
    struct TupleValue
    {
        name @0 :Text;
        value @1 :Value;
    }

    data :union
    {
        void @0 :Void;
        bool @1 :Bool;
        int8 @2 :Int8;
        int16 @3 :Int16;
        int32 @4 :Int32;
        int64 @5 :Int64;
        uint8 @6 :UInt8;
        uint16 @7 :UInt16;
        uint32 @8 :UInt32;
        uint64 @9 :UInt64;
        float32 @10 :Float32;
        float64 @11 :Float64;
        blob @12 :Data;
        string @13 :Text;
        array @14 :List(Value);
        list @15 :List(Value);
        enum @16 :UInt16;
        tuple @17 :List(Value.TupleValue);
        interface @18 :Void;
        anyPointer @19 :Void;
    }
}

struct Attribute
{
    id @0 :UInt64;
    value @1 :Value;
}

struct Enumerator
{
    name @0 :Text;
    declOrder @1 :UInt16;   # implicit based on order it was written in the schema file
    ordinal @2 :UInt16;     # explicit ordinal value specified using @

    attributes @3 :List(Attribute);
}

struct Field
{
    name @0 :Text;
    declOrder @1 :UInt16;   # implicit based on order it was written in the schema file
    unionTag @2 :UInt16;    # tag value for this field if it is in a union

    attributes @3 :List(Attribute);

    meta :union
    {
        normal :group
        {
            ordinal @4 :UInt16; # explicit ordinal value specified using @
            index @5 :UInt16;   # implicit index of the field in its section

            type @6 :Type;
            defaultValue @7 :Value;

            # For data fields this is the offset in units of the field size from the beginning of
            # the data section (after the metadata). For example, for a uint32 field multiply this
            # by 4 to get the byte offset.
            # Always zero for pointer fields.
            dataOffset @8 :UInt32;
        }

        group :group
        {
            typeId @9 :UInt64; # Id of the type that defines the group's structure
        }

        union :group
        {
            typeId @10 :UInt64; # Id of the type that defines the union's structure
        }
    }
}

struct Method
{
    name @0 :Text;
    declOrder @1 :UInt16;    # implicit based on order it was written in the schema file
    ordinal @2 :UInt16;      # explicit ordinal value specified using @

    paramStruct @3 :UInt64;
    resultStruct @4 :UInt64;

    attributes @5 :List(Attribute);
    typeParams @6 :List(Text);
}

struct SourceInfo
{
    docComment @0 :Text;
    file @1 :Text;
    line @2 :UInt32;
    column @3 :UInt32;
}

struct Declaration
{
    name @0 :Text;
    id @1 :UInt64;
    parentId @2 :UInt64;

    source @3 :SourceInfo;

    attributes @4 :List(Attribute);
    typeParams @5 :List(Text);

    children @6 :List(Declaration);

    data :union
    {
        file @7 :Void;

        attribute :group
        {
            type @8 :Type;

            targetsAttribute @9 :Bool;
            targetsConstant @10 :Bool;
            targetsEnum @11 :Bool;
            targetsEnumerator @12 :Bool;
            targetsField @13 :Bool;
            targetsFile @14 :Bool;
            targetsInterface @15 :Bool;
            targetsMethod @16 :Bool;
            targetsParameter @17 :Bool;
            targetsStruct @18 :Bool;
        }

        constant :group
        {
            type @19 :Type;
            value @20 :Value;
        }

        enum :group
        {
            enumerators @21 :List(Enumerator);
        }

        interface :group
        {
            super @22 :Type;
            methods @23 :List(Method);
        }

        struct :group
        {
            dataFieldCount @24 :UInt16;
            dataWordSize @25 :UInt16;
            pointerCount @26 :UInt16;
            isGroup @27 :Bool;
            isUnion @28 :Bool;
            isMethodParams @29 :Bool;
            isMethodResults @30 :Bool;
            unionTagOffset  @31 :UInt32;   # offset to union tag, in 2-byte units, if isUnion is true.
            fields @32 :List(Field);
        }
    }
}

struct Import
{
    path @0 :Text;
    schema @1 :SchemaFile;
}

struct SchemaFile
{
    root @0 :Declaration;
    imports @1 :List(Import);
}

struct Tester
{
    data :union
    {
        a @0 :Bool;
        c @2 :UInt64;
        derp2 :group
        {
            h @7 :Import;
        }
        k @10 :UInt32;
        derp :group
        {
            n @13 :Import;
            p @15 :Import;
            herp :union
            {
                lerp :group
                {
                    q @16 :UInt16;
                    r @17 :UInt16;
                }

                herp :group
                {
                    s @18 :UInt32;
                    t @19 :Import;
                }

                berp :group
                {
                    u @20 :Import;
                    v @21 :UInt32;
                }
            }
        }
    }

    data2 :union
    {
        d @3 :Import;
        f @5 :SchemaFile;
        i @8 :Import;
        derp :group
        {
            l @11 :UInt32;
            m @12 :UInt32;
        }
    }

    b @1 :UInt64;
    e @4 :Import;
    g @6 :SchemaFile;
    j @9 :UInt32;
    o @14 :Import;
}

struct Tester2
{
    data :union
    {
        c @0 :UInt64;
        herp :union
        {
            lerp :group
            {
                q @1 :UInt16;
                r @2 :UInt16;
            }
            s @3 :UInt32;
        }
    }
}
