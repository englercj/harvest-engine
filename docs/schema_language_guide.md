# Harvest Schema - Language Guide

TODO Doc: pointers

## Basic Example

```c
import "file.schema";

namespace he.editor.schema;

enum Stuff : uint8
{
    Value0,
    Value1,
    Etc,
}

struct Vector3
{
    x: float;
    y: float;
    z: float;
}
```

## Comments

C-style single line comments are allowed:

```c
// This is a comment
```

## Imports

Other schema files can be imported using the `import` declaration.

```c
import "other.schema";
import "some/path/another.schema";
```

Imports must be the first non-comment statements in the file.

## Namespace

A schema file may specify a namespace:

```c
namespace he.schema.example;

// has a FQN of `he.schema.example.A`
struct A {}
```

A namespace, if specified, must be the first statement after imports in the file. A schema file may only specify a single namespace per file.

## Built-in Types

The following types are defined as part of the language.

### Basic Types

|   Schema   |      C++      |    C#    | Size (bytes) |
| ---------- | ------------- | -------- | -----------: |
| `bool`     | `bool`        | `bool`   |            1 |
| `int8`     | `int8_t`      | `sbyte`  |            1 |
| `int16`    | `int16_t`     | `short`  |            2 |
| `int32`    | `int32_t`     | `int`    |            4 |
| `int64`    | `int64_t`     | `long`   |            8 |
| `uint8`    | `uint8_t`     | `byte`   |            1 |
| `uint16`   | `uint16_t`    | `ushort` |            2 |
| `uint32`   | `uint32_t`    | `uint`   |            4 |
| `uint64`   | `uint64_t`    | `ulong`  |            8 |
| `float32`  | `float`       | `float`  |            4 |
| `float64`  | `double`      | `double` |            8 |

### Containers

|    Schema    |             C++             |         C#         |     Notes     |
| ------------ | --------------------------- | ------------------ | ------------- |
| `T[]`        | `he::Vector<T>`             | `List<T>`          | Dynamic array of elements `T` |
| `T[N]`       | `T[N]`                      | `T[N]`             | Fixed array of elements `T` of size `N` |
| `string`     | `he::String`                | `String`           | Dynamic string which may only hold UTF-8 or 7-bit ASCII |
| `list<T>`    | `std::list<T>`              | `LinkedList<T>`    | Doubly linked list of elements `T` |
| `set<T>`     | `std::unordered_set<T>`     | `HashSet<T>`       | Unordered set of unique elements `T` |
| `map<K, T>`  | `std::unordered_map<K, T>`  | `Dictionary<K, T>` | Unordered hash map which associates keys `K` to elements `T` |

## Structs

A collection of named and typed fields. Cannot contain methods. Can extend a single struct, which inherits the fields.

```c
struct Base {}

struct Child extends Base
{
    fieldName: uint32 = 15;
}
```

Default values can also initialize structures:

```c
struct Vec3
{
    x: float;
    y: float;
    z: float;
}

struct Transform
{
    pos: Vec3 = { 0, 1, 2 };
    rot: float[3] = { 0, 1, 2 };
}
```

Struct can also be nested:

```c
struct List
{
    struct Node
    {
        next: Node;
        prev: Node;
    }

    first: Node;
}
```

Structures do not have to be defined before they are used, but they have to be defined at some point in the file:

```c
struct User
{
    UserId id;
}

struct UserId
{
    bytes: uint8[16];
}
```

## Enums

A type for listing symbolic values.

```c
enum Something
{
    Foo,
    Bar,
    Baz,
}
```

The default underlying type of an enum is `int32`, but you can specify any integral type you wish.

```c
enum Something : uint16
{
    Foo,
    Bar,
    Baz,
}
```

Enumerant values are always the previous value plus one, with the first item defaulting to a value of zero. You can also specify the values manually if you wish.

```c
enum Something : uint16
{
    Foo,        // Defaults to 0 as the first element
    Bar,        // Has a value of 1 (Foo + 1)
    Baz = 6,    // Has a value of 6
    Qux,        // Has a value of 7 (Baz + 1)
}
```

## Constants

Constants define a literal value that is output into generated code.

```c
const Foo: int32 = 123;
const Bar: Vec3 = { 0, 1, 2 };
```

## Interfaces

A collection of named and typed methods. Cannot contain fields. Can implement multiple interfaces, which inherits the method definitions.

```c
interface Metal
{
    GetMetalType() -> uint32;
}

interface Fuel
{
    SetFuelType(fuel: uint32) -> fuel;
}

interface Car implements Metal, Fuel
{
    Drive(from: Vec3, to: Vec3);
}
```

## Generics

Structs, interfaces, and type aliases can be parameterized with one or more type paramters. When instantiating a generic struct all type parameters must be concrete types.

```c
struct Simple
{
    b: bool;
}

struct Example<T, U> extends T
{
    field: U;
}

struct Usage
{
    example: Example<Simple, int32>;
}
```

## Type aliases

A type alias is basically an alternative name for a type. Type aliases can have generic type parameters.

```c
using Vec3<T> = T[3];
using Vec3f = Vec3<float>;
```

## Services

Similar to interfaces but has different codegen, can also implement interfaces. Probably generates GRPC stuff.

```c
interface UniqueIdGenerator
{
    CreateUniqueId() -> uint64;
}

service Player implements UniqueIdGenerator
{
    SetUserName(id: uint64, name: string) -> bool;
    ReadActions(id: uint64) -> stream Action;
    LogMessages(logs: stream string) -> stream bool;
}

// Services can also be generic
service ApiGateway<T>
{
    Auth(token: T) -> AuthResult;
}
```

## Attributes

Custom attributes:

```c
// Declare an attribute that applies to struct fields and stores a string value.
attribute Foo(field): string;

// Declare an attribute that applies to any construct and stores a uint32 value.
attribute Bar: uint32;

// Declare an attribute that applies to structs or interfaces and has no value.
attribute Baz(struct, interface);

// Declare an attribute that applies to any construct and has no value.
attribute Qux;

// Example usage of the attributes
[Bar(0)]
[Baz]
[Qux]
struct MyStruct
{
    [Foo("test")]
    [Bar(1)]
    [Qux]
    fieldName: uint32 = 0;
}
```

Valid values for the attribute target are:

- file
- struct
- field
- enum
- enumerator
- interface
- method
- parameter
- const
