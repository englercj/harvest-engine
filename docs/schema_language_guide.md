# Harvest Schema - Language Guide

Originally based on the language of [Cap'n Proto](https://capnproto.org/).

## Why Does Schema Exist?

TODO: What is this? Why does it exist? Comparison with other options (Protobuf, Flatbuffers, bond, Cap'n Proto, etc).

Quickly:

- Protobuf: serialize/deserialize step is expensive, std proliferation is big sad, unknown fields aren't supported for json serialization, no default values.
- Flatbuffers: mutate and reflection APIs are weak, no unknown field support, no defaults for struct fields, fields that are unset or set to default are stored the same.
- Bond: Compiler is Haskell, library relies on boost, very complex (but feature rich), no defaults for struct fields.
- Capnp: API is very difficult to use, unknown fields aren't supported for json serialization, uses exceptions and RTTI, comes with a *lot* of libraries (including KJ), fields that are unset or set to default are stored the same, default value can never change.

The goals of Harvest Schema are:

- Zero-copy binary format that is used in-memory and on disk. Data can be read with `mmap`.
- Support for unknown fields in both binary and text representations, and when serializing between them.
- Ability to distinguish between unset fields and fields set to the default value.
- Allow the default value of fields to change while continuing to properly load old "set" data.
- Robust reflection and mutation APIs so the data can easily be changed at edit time.

Unfortunately while many libraries hit some of these points, none of them hit them all. It was decided that these points are too important to compromise on.

## Schema Evolution

The changes that remain backwards compatible depend on the encoding format being used. Because of this individual encoding formats will document their own schema evolution rules.

## Basic Example

```c++
import "file.he_schema";

namespace he.editor.schema;

enum Stuff :uint8
{
    Value0,
    Value1,
    Etc,
}

struct Vector3
{
    x @0 :float32;
    y @1 :float32;
    z @2 :float32;
}
```

## Language Reference

### Comments

Comments are denoted by two slashes (`//`) and extend until the end of the line:

```c++
// This is a comment
```

### Unique IDs

A schema file *must* specify a unique 64-bit ID, and each type therein *may* specify an ID. The compiler tool can generate a unique ID randomly with the `schemac id` command. ID specifiers begin with `@` followed by a numerical value, usually in hexadecimal:

```
@0xa2843c7c246bddb4

struct Foo @0x98b74b6f4f926724 {}
enum Bar @0xcd45163cf158e144 {}
interface Baz @0xbd975406d578029c {}
attribute Qux @0xa7aa31a681933d0e (field) :String;
```

A file's unique ID must be the first non-comment statement.

If you omit the ID for a type one will be assigned automatically from an FNV-1a 64-bit hash of the parent scope's ID and the declaration's name. Because of this, renaming or moving a type that does not explicitly specify the ID will generate a new one. Before performing any such operation use the `schemac inspect` command which will echo the schema with additional information, such as the ID of your types. Make sure to specify that same ID after moving or renaming a type to maintain compatibility.

### Imports

Other schema files can be imported using the `import` declaration. Importing a file makes all the symbols declared in that file available for use in the current file.

```c++
import "other.he_schema";
import "some/path/another.he_schema";
```

Imports, if specified, must be the first non-comment statements after the file's unique ID. The compiler will search for imports first next to the schema file being compiled, then in the search paths specified with `-I`.

### Namespace

A schema file may specify a single namespace:

```c++
namespace he.schema.example;

// has a FQN of `he.schema.example.A`
struct A {}
```

Only one namespace may be specified per file. Though not required, it is recommended that the namespace be the first non-comment statement after imports in the file.

### Types

#### Build-in Types

The following types are defined as part of the language:

- `void` - Has exactly one possible value, and therefore encodes with zero bits; useful in unions.
- `bool` - Has two possible values: true, and false.
- `int8`, `int16`, `int32`, `int64` - Signed integral values that are 1, 2, 4, and 8 bytes in size, respectively.
- `uint8`, `uint16`, `uint32`, `uint64` - Unsigned integral values that are 1, 2, 4, and 8 bytes in size, respectively.
- `float32`, `float64` - Floating-point values that are 4 and 8 bytes in size, respectively.
- `List<T>` - A variable sized list of values of type `T`.
- `String` - A variable sized list of UTF-8 characters terminated by a null (zero) byte.
- `Blob` - A variable sized sequence of bytes. Similar to `List<uint8>` but also allows for a special byte-string syntax for values.

#### Fixed-Size Arrays

Any type can be in a fixed-size array by using the `[N]` syntax, where `N` is the number of elements in the array. The size of a fixed-size array is limited to 65,535. Multi-dimensional arrays are not allowed.

```c++
struct Vec3
{
    v @0 :float32[3];
}
```

The binary encoding for this type is equivalent to:

```c++
struct Vec3
{
    v0 @0 :float32;
    v1 @1 :float32;
    v2 @2 :float32;
}
```

Changing the size of a fixed-size array is not backwards-compatible for the binary format, just as changing the type of any field is not backwards-compatible. Other formats may treat these types differently, and have different restrictions about changes.

### Type Aliases

Aliases of types can be created using the `alias` keyword. Aliases are a language-level feature, and don't affect codegen. In fact, the schema wont even include aliases; only their resolved types are included.

```c++
alias Vec3 = float32[3];
```

### Structs

A collection of named and typed fields which are numbered consecutively starting from zero.

```c++
struct Vec3
{
    x @0 :float32;
    y @1 :float32;
    z @2 :float32;
}
```

Fields may also have default values:

```c++
struct Values
{
    foo @0 :int32 = 123;
    bar @1 :String = "blah";
    baz @2 :List<bool> = [true, false, false, true];
    qux @3 :Vec3 = { x = 0, z = 2 };

    fixed @4 :float32[4] = [1, 2, 3, 4];
    data @5 :Blob = 0x"a1 40 33";
    none @6 :void; // void cannot have a default value
}
```

#### Field Ordinals

Every field has a numerical value that represents its order in the evolution of the schema. This value is specified with an `@` followed by a decimal number which starts at zero and increments by one without any gaps.

These ordinals define the evolution of the schema. You should never remove or change fields that have an ordinal value, and new fields should always use the next integer ordinal value.

#### Nesting

Constants and type declarations can be nested inside structs and interfaces. Doing so simply acts as a namespace for the type. You can refer to types in another scope by listing the levels of the scope separated with `.`.

```c++
struct Foo
{
    struct Bar {}
    bar @0 :Bar;
}

struct Baz
{
    bar @0 :Foo.Bar;
}
```

### Unions

A collection of two or more named and typed fields of which only one can be set. Unions in Harvest Schema are tagged, meaning there is a tag value that stores which field is set. Unlike in C, unions are not separate types. They can only be declared within a struct:

```c++
struct Shape
{
    value :union
    {
        u64 @0 :int64;
        u32 @1 :int32;
        u16 @2 :int16;
        u8 @3 :int8;
        b @4 :bool;
        empty @5 :void;
    }
}
```

Notes:

- Unions are named, but do not have an ordinal value. That is because they aren't a "real" field but instead just a namespace that fields are collected in.
- Fields in a union are numbered in the same space as fields of the containing struct. This informs the system of the union field's evolution relative to other fields of the struct.
- In this example, the void type is used as a member. This field stores no information, but the tag can let us know that `empty` is the set field which can be useful.
- By default, when a struct is initialized the first union field (that is the field with the lowest ordinal) is considered "set".

### Enums

A type with a small set of symbolic values. Like struct fields, enums must specify an ordinal that represents the schema evolution. This ordinal value also gets used as the enum's value in the code generation for some languages.

```c++
enum Something
{
    Foo @0;
    Bar @1;
    Baz @2;
}
```

### Constants

Constants define a literal value that is output into generated code.

```c++
const Foo :int32 = 123;
const Bar :Vec3 = { x = 0, y = 1, z = 2 };
const Key :Blob = 0x"23d8edbd1ea57d74 aa59328790523ef";
```

Constants can be used in other constants or as the default value for fields:

```c++
const Foo :int32 = 123;
const Bar :String = "Hello";
const Baz :SomeStruct = { id = Foo, message = Bar };
```

### Interfaces

A collection of methods, each of which takes a structure as input and returns a structure as output. Like struct fields, methods have ordinal numbers. Interfaces also support inheritance.

```c++
interface Node
{
    // Methods can use tuples to define inline structures for parameters and results. Items in a
    // tuple are specified just as fields are, except that the ordinal value is implicit based on
    // declaration order.
    // If you must reorder parameters later be sure to explicitly specify the ordinal values to
    // maintain backwards compatibility with older schemas.
    IsDirectory @0 () -> (result: bool);

    // Or if you prefer to use explicit structure types, there is an alternative syntax for that.
    // This can be useful if there are commonly reused structures, or very large structures that
    // can improve readability by being split out.
    // The structures used in IsDirectory2 are equivalent to the ones generated for IsDirectory.
    IsDirectory2 @1 :IsDirParams -> :IsDirResult;

    // Or even a mix
    IsDirectory3 @2 () -> :IsDirResult;

    struct IsDirParams
    {

    }

    struct IsDirResult
    {
        result @0 :bool;
    }
}

interface Directory extends Node
{
    // Interfaces can be used as parameters or return values which are serialized as references to an instance of that interface in the sender.
    CreateFile @1 (name :String) -> (file :File);
}

interface File extends Node
{
    // Default values can be given to parameters or return values, because tuple fields are just struct fields.
    Read @0 (offset :uint64 = 0, size :uint64 = 0) -> (data :Blob);

    // You can omit the return type to indicate a fire-and-forget call which gets no response.
    Write @1 (offset :uint64 = 0, data :Blob);

    // The `stream` keyword indicates that a series of ordered values will be sent or received.
    ReadStream @2 (offset :uint64 = 0, size :uint64 = 0) -> stream (data :Blob);
    WriteStream @3 stream (offset :uint64 = 0, data :Blob);
}
```

### Generics

Structs, Interfaces, and Methods be generic, meaning they have type parameters. For example:

```c++
struct Map<Key, Value>
{
    entries @0 :List<Entry>;

    struct Entry
    {
        key @0 :Key;
        value @1 :Value;
    }
}

struct People
{
    nameMap @0 :Map<String, Person>;
}
```

Generics are similar to C# generics. However, only pointer types (structs, lists, and interfaces) can be used as generic parameters. This is because primitive generic parameters would mean different specializations of a struct would have different layouts. This would significantly complicate the implementation.

Methods in an interface can also be generic:

```c++
interface Assignable<T>
{
    Get @0 () -> T;
    Set @1 (value :T);
}

interface AssignableFactory
{
    Create<T> @0 (initialValue :T) -> Assignable<T>;
}
```

### Attributes

Attributes attach additional information to schema definitions. These attributes are then available in the generated schema so they can affect code generation or runtime behavior. Attributes are always specified after the thing they modify, but before any sub-blocks. File-level attributes can be specified anywhere in the top-level scope.

Attributes can be declared using the `attribute` keyword:

```c++
// Declare an attribute `Foo` that can attach to struct fields and stores a string value.
attribute Foo(field) :String;

// Declare an attribute that applies to any construct and stores a uint32 value.
attribute Bar(*) :uint32;

// Declare an attribute that applies to structs or interfaces and stores no value.
attribute Baz(struct, interface) :void;

// Declare an attribute that applies to any construct and stores a struct value.
struct QuxData
{
    a @0 :String;
    b @1 :int32;
}
attribute Qux(*) :QuxData;

// File-level attributes are each on their own line.
$Bar(0);
$Qux(a = "No", b = 2);

// Attributes on declarations come after the name, but before the block.
enum Thing $Bar(1)
{
    Value @0 $Bar(2);
}

// Example usage of many of the attributes.
struct MyStruct $Bar(3) $Baz $Qux(a = "Yes", b = 1)
{
    fieldName :uint32 = 0 $Foo("test") $Bar(4);
}
```

Valid values for the attribute target are:

- `attribute`
- `const`
- `enum`
- `enumerator`
- `field`
- `file`
- `interface`
- `method`
- `parameter`
- `struct`

There are a few built-in attributes that can affect code generation:

- `$Flags` Marks an enum as a series of flags. See [Enums](#Enums) section for details.
