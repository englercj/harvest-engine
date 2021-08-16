# Harvest Schema - Language Guide

TODO: Pointers vs inline? Does it default to inline and support `field: Node*;`? Or does it default to pointer and support `[CppInline] field: Node;`?

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

## Built-in Types

The following types are defiend as part of the language.

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
| `T[]`        | `std::vector<T>`            | `List<T>`          | Dynamic array of elements `T` |
| `T[N]`       | `std::array<T, N>`          | `T[N]`             | Fixed array of elements `T` of size `N` |
| `string`     | `std::string`               | `String`           | Dynamic string which may only hold UTF-8 or 7-bit ASCII |
| `list<T>`    | `std::list<T>`              | `LinkedList<T>`    | Doubly linked list of elements `T` |
| `set<T>`     | `std::unordered_set<T>`     | `HashSet<T>`       | Set of unique elements `T` |
| `map<K, T>`  | `std::unordered_map<K, T>`  | `Dictionary<K, T>` | Unordered hash map which associates keys `K` to elements `T` |

## Structs

A collection of named and typed fields.

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

## Enums

A type for listing symbolic values. Follows the same rules as C for determining the numeric value.

```c
enum Something
{
    Foo,
    Bar,
    Baz,
}
```

## Constants

Constants define a literal value that is output into generated code.

```c
const Foo: int32 = 123;
const Bar: Vec3 = { 0, 1, 2 };
```

## Interfaces

A collection of named and typed methods.

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

Structs and interfaces can be parameterized with one or more type paramters.

```c
struct Example<T, U> extends T
{
    field: U;
}
```

The usage of a type parameter within a generic struct definition may implicitly constrain what type(s) can be used to instantiate the generic struct:

```c
struct Example<T>
{
    // The default value of 10 implicitly constrains T to numeric types
    x: T = 10;
}
```

You can also explicitly constrain the type parameter

## Type aliases

C++-style type aliases

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
