# C++ Style Guide

This document contains a set of guidelines for writing C++ code in Harvest.

## Table of Contents

- [General](#general)
- [EditorConfig](#editorconfig)
- [Whitespace and Braces](#whitespace-and-braces)
- [Comments](#comments)
- [Files](#files)
- [Types](#types)
    * [Basic Types](#basic-types)
    * [Strings](#strings)
    * [Classes and Structs](#classes-and-structs)
    * [Enums](#enums)
    * [Type aliases](#type-aliases)
    * [Casting](#casting)
    * [Null](#null)
- [Namespaces](#namespaces)
- [Functions](#functions)
    * [Parameters](#parameters)
    * [Template Parameters](#template-parameters)
- [Variables and Constants](#variables-and-constants)
- [Preprocessor](#preprocessor)
    * [Fully Qualified Names](#fully-qualified-names)
    * [Includes](#includes)
- [Control Flow](#control-flow)
    * [Conditionals and Loops](#conditionals-and-loops)
    * [Switch-Case](#switch-case)
- [Thread-Safety](#thread-safety)
- [Error Handling](#error-handling)
    * [Exposing Errors](#exposing-errors)
    * [When to use `HE_ASSERT`](#when-to-use-he_assert)
    * [When to use `HE_VERIFY`](#when-to-use-he_verify)
    * [When to use `HE_LOG_ERROR`](#when-to-use-he_log_error)
- [Appendix](#appendix)
    * [Why avoid STL headers?](#why-avoid-stl-headers)
    * [Discouraged Features](#discouraged-features)

## General

- Use standard C++20 and avoid non-standard extensions
- Avoid STL headers when possible ([details](#why-avoid-stl-headers))
- Avoid [Discouraged Features](#discouraged-features)
- Don't rely on operator precedence. Always use parenthesis when using multiple operators
- Source code has a soft length max at column 150
- Use `const` for variables, functions, and parameters; unless you can't
    * Exception: Pointers to const-data is acceptable. E.g: `const char* A;` is OK, don't need `const char* const A;`
    * Exception: Pass-by-value parameters don't need to be marked `const`
- For pointers place the `const` before the type: `const char*`

## EditorConfig

- Whatever editor you use should utilize EditorConfig
- Detailed formatting options for Visual Studio are in the `.editorconfig` file

## Whitespace and Braces

- All indentation should use 4 spaces
- Use Allman style braces, which puts open and close braces on their own line
- Expressions with parenthesis should not have a space between values and the parenthesis
- Aggregate initializers should have a space between values and the braces
- Operators always have a space on either side

## Comments

- Comments have a hard length max at column 100
- Prefer single-line comments (`//`) to multi-line comments (`/* */`)
- Use documentation comments (`///`) for doxygen comments
- Use comments to explain "why" rather than "what", unless the code/flow is non-obvious
- Use separator comments (`// -------------`) that end at column 100 to section off logically related code

## Files

- File names should be `lower_snake_case` and match the name of the primary class they declare
    * If no primary class, a reasonable name for the collection of utilities or types
    * E.g.: `string.h` declares `he::String`
    * E.g.: `file_loader.h` declares `he::FileLoader`
- Start all files with a simple copyright comment as their first line (`// Copyright Chad Engler`)
- Headers should end with `.h`
- Source files should end with `.cpp`
- Inline implementation files should end with `.inl`
- Program entry-points should be in a `<name>_main.cpp` file.
    * Omit the `he_` prefix for Harvest modules
    * E.g.: module `he_bin2c` uses `bin2c_main.cpp`
- A module entry-point should be in a `<name>_module.cpp` file.
    * Omit the `he_` prefix for Harvest modules
    * E.g.: module `he_assets_editor` uses `assets_editor_module.cpp`
- Unit tests are in files named `test_<name>.cpp`.
    * `<name>` should be the name of the file being tested (without extension)
    * E.g.: file `he/core/string.h` is tested by `test_string.cpp`
- Unit test names should follow the following format:
    * Module name: name of the module without `he_` prefix
    * Suite name: name of the file being tested (without extension)
    * Test name: name of the class function being tested, or a name of the flow being tested
    * E.g.: When testing `he::String::Append()` from `he/core/string.h` the name should be `HE_TEST(core, string, Append)`
- For platform-specific code use file suffixes (`*.win32.cpp`, `*.posix.cpp`, etc) and minimal `#if` branching

## Types

### Basic Types

- Use fixed-width integer values (`uint8_t`, `int32_t`)
- Prefer unsigned over signed when the value should never be negative
- Use `uint8_t` to represent bytes/blobs (`uint8_t[]`)
- Use `size_t` for memory region sizes, and `uint32_t` for element counts
    * Memory allocators may deal with sizes > 4B, so use `size_t`
    * Vectors shouldn't deal with sizes > 4B, so use `uint32_t`
- Use `void*` for memory buffers where type is irrelevant
- Prefer hard types (`struct Id { uint32_t val; };`) over typedefs (`using Id = uint32_t;`)

### Strings

- Strings should be `char` sequences (not `wchar_t`)
- All strings are assumed to be UTF-8 encoded, unless otherwise documented

### Classes and Structs

- Use `UpperCamelCase` for class and struct names
- Structs should be [Aggregates](https://en.cppreference.com/w/cpp/language/aggregate_initialization)
    * Use `lowerCamelCase` for non-static data members of structs with no prefix
    * Avoid polymorphism (use a class instead)
    * Avoid member functions (operators or constructors are OK)
- Classes should be used for encapsulation where it makes sense
    * Use `lowerCamelCase` for non-static data members of classes with an `m_` prefix
- Prefer to mark constructors and destructors as `noexcept`
    * Exceptions are not enabled in Harvest by default
    * Also include it for `= default;` declarations
- Use inline initializers for members, using universal initialization syntax (`{}`).
    * Exception: When a member is initialized by a constructor parameter
    * Exception: When a struct or class member has a default constructor
- Do not call virtual functions from constructors
- Use `explicit` for constructors with a single parameter
    * Exception: copy/move ctors
- Follow the [Rule of Five](https://en.cppreference.com/w/cpp/language/rule_of_three) for constructors
    * Polymorphic classes should disable copy semantics
- Make members private by default, and take care for exposed data
- Use access specifiers to group similar things together in this order (all this public, then protected, then private):
    1. Types and type aliases (`using`, `enum class`, `struct`, etc)
    2. Static constants
    3. Static functions
    4. Constructors and operators
    5. Destructor
    6. Non-static member functions
    7. Data members

#### Getters

Getters should use the name of what they get without any prefix.

Example:

```cpp
class Example
{
public:
    uint32_t Value() const { return m_value; }

private:
    uint32_t m_value;
}
```

If a getter for a boolean it should be prefixed with `Is` or `Are`:

```cpp
class Dialog
{
public:
    bool IsClosing() const { return m_closing; }

private:
    bool m_closing{ false };
};
```

If a getter takes any parameters, or if the getter has the same name as the type it returns then the function should be prefixed with `Get`:

```cpp
enum ElementSize { Small, Big };

class Example
{
public:
    ElementSize GetElementSize() const { return m_elementSize; }

private:
    ElementSize m_elementSize;
}
```

#### Setters

TODO: review for correctness

Setters should use the name of what they set with a `Set` prefix.

Example:

```cpp
class Example
{
public:
    void SetValue(uint32_t value) { m_value = value; }

private:
    uint32_t m_value;
}
```

#### Template Parameters

- Prefer the `typename` keyword to `class` in template parameter lists
- Use simple names for template parameters where usage is obvious
    * E.g. `<typename T>` or `<typename T, typename U>`
- Use a name prefixed with `T` when naming a template parameter improves clarity
    * E.g. `<typename TKey, typename TValue>`

### Enums

- Use `UpperCamelCase` for enum names
- Prefer scoped enums (`enum class`), not unscoped enums (`enum`)
- Use the `HE_ENUM_FLAGS` macro to enable bitwise operations on enum types

### Type aliases

- Use `UpperCamelCase` for type alias names
- Prefer `using` syntax instead of `typedef`

### Casting

- Prefer C++-style casting in all cases
- Avoid using `dynamic_cast`
    * Exceptions are not enabled in Harvest by default
- Avoid using c-style casts (`(int)x`) or constructor-style casts (`int(x)`)
- Generally try to use `static_cast`, unless you can't

### Null

- Use `nullptr` for null pointers instead of `NULL` or `0`
- Use `'\0'` for the null characters instead of `0`

## Namespaces

- Use `lower_snake_case` for namespace names
- Try to match module names to the namespace they provide
    * E.g. The `he_schema` module provides the `he::schema` namespace
    * E.g. The `he_assets` module provides the `he::assets` namespace
    * One exception is `he_core` which provides the root `he` namespace
- Prefer C++17 nested namespace definitions (`namespace he::assets {}`)
- Code within a namespace should be indented by one

## Functions

- Use `UpperCamelCase` for function names
- Lambdas are allowed, but generally named functions should be preferred
    * Simple iterators or predicates that are immediately passed into a function are examples of good sues for lambdas
- Avoid function pointer parameters, instead prefer `he::Delegate` or a template parameter
    * Templates for callbacks are preferred if the callback is invoked immediately, like an iterator.
- All params should be declared on one line, or all params separated by newlines with one indent
    * Do not split params across multiple lines unless you split *all* params on multiple lines
    * Same for function calls, all on one line or all on separate lines
- Omit unused parameter names when their use is obvious (like a deleted copy constructor)
    * Otherwise write the name and use `[[maybe_unused]]` to avoid warnings
- Pure functions should be marked `[[nodiscard]]`
    * With no side effect, there is no reason to call a pure function and not use the return value

### Parameters

- Use `lowerCamelCase` for parameter names
- Destination parameters, aka out parameters, should be the first parameter
    * Destination first, count/size of buffers directly after, then everything else
    * Similar to the common `memcpy(dst, src, len)` pattern

### Template Parameters

- Use `UpperCamelCase` for template parameter names
- Prefer the `typename` keyword to `class` in template parameter lists
- Use simple names for template parameters where usage is obvious
    * E.g. `<typename T>` or `<typename T, typename U>`
- Use a name prefixed with `T` when naming a template parameter improves clarity
    * E.g. `<typename TKey, typename TValue>`

## Variables and Constants

- Use `lowerCamelCase` for variable names
- Use `UpperCamelCase` for compile-time constant names
- Do not use Hungarian notation
- Always initialize member variables of basic types
- Always initialize block-scoped variables, even if you pass it somewhere immediately for writing

## Preprocessor

- Use `UPPER_SNAKE_CASE` for macro names
- Use a project-specific prefix for macro names
    * E.g. All macros provided by the engine use the `HE_` prefix
- Indent nested preprocessor directives like normal code
- Start indentation of preprocessor directives one indent left of surrounding code
- Macros with multiple statements should be encoded in `do { } while (0)`
    * Exception: if the macro needs to return the value of the statements
- Function-like macros should not end in a semicolon, so the caller of a function-like macro can specify one
- Prefer functions and templates to function-like macros
- Try to avoid using conditional compilation where possible
- Use `#if defined()` instead of `#ifdef` and `#if !defined()` instead of `#ifndef`
    * Avoid conditionally defining macros, prefer defining them as `0` or `1` instead

Examples:

```cpp
#define HE_MACRO_EXAMPLE 1

#if HE_MACRO_EXAMPLE
    #if HE_ENABLE_ASSERTIONS
        #include "something.h"
    #endif
#endif

int DoStuff()
{
#if HE_MACRO_EXAMPLE
    #if HE_ENABLE_ASSERTIONS
        return 0;
    #else
        return -1;
    #endif
#else
    return 1;
#endif
}
```

### Fully Qualified Names

All code generated by macros should use fully qualified names, including the global `::` prefix.

Macros can be used in any namespace context, so always using the fully qualified name for types ensures the macro works in any context.

Example:

```cpp
#define HE_MAKE_STRING(name, value) ::he::String name(#value)
```

### Includes

- All includes, including from contrib packages, should use quotes (`""`)
- All system includes, such as STL, should use angle brackets (`<>`)
- Include what you use, don't rely on transient includes
- Forward declare when possible to reduce include tree depth
    * Exception: Don't try to forward declare templates, unless they are very simple
- Use `#pragma once` rather than header guard macros

#### Include Grouping

Includes should be grouped by where they are coming from. Within each group they should be ordered alphabetically. The definition, and order, of the groups is:

1. Header for this implementation (if .cpp)
2. Local includes in your module implementation (src/)
3. Module includes, including the public includes of your own module (include/)
4. Contrib includes
5. System includes

Example `allocator.cpp` file:

```cpp
// Copyright Chad Engler

#include "he/core/allocator.h"

#include "allocator_helpers.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/utils.h"

#include "zlib.h"

#include <new>
```

## Control Flow

### Conditionals and Loops

- Conditions and loops should always have braces for their body
- Multiline conditionals should lead with the operator (i.e.: start with `&&` or `||`)

### Switch-Case

- Indent case statements
- Add a block scope if you declare any variables in a case body
- Add a block scope if the logic is more complex than a few simple statements

## Thread-Safety

- Functions are assumed to be not thread-safe, unless otherwise documented.
- Objects are assumed to be not thread-safe, unless otherwise documented.

## Error Handling

### Exposing Errors

Errors should be expressed through a boolean or enum return value. Use a bool unless the type of error would influence the behavior of a caller, in which case use an enum.

For platform abstractions where the implementation will be interfacing with the Operating System use the `he::Result` structure which represents an operating system result code.

### When to use `HE_ASSERT`

The `HE_ASSERT` macro is used to validate preconditions that MUST be true. That is, without the condition being true the program must immediately halt either because continuing would crash anyway or because continuing would cause data corruption. Generally speaking assertions should be used to validate *programmer* assumptions, not *user* assumptions nor data.

- **DO** use `HE_ASSERT` to validate array bounds access and other memory overwrite errors.
- **DO NOT** use `HE_ASSERT` to validate function parameters are within expected values (use `HE_VERIFY` instead).
- **DO NOT** use `HE_ASSERT` to validate data that can comes from user input, data files, network, etc.

`HE_ASSERT` is compiled out of the program when `HE_ENABLE_ASSERTIONS=0`. By default, the Release configuration will define `HE_ENABLE_ASSERTIONS=0` and therefore strip assertions from the build.

### When to use `HE_VERIFY`

The `HE_VERIFY` macro is used to validate preconditions that SHOULD be true. That is, a bug is present if the condition is not true but the program can recover. Most commonly `HE_VERIFY` is used to validate expected state or input which when incorrect can lead to bad behavior. However since the program can continue, it does so after logging the issue.

- **DO** use `HE_VERIFY` to validate function parameters are within expected values.
- **DO** use `HE_VERIFY` to validate data members are in a valid state.
- **DO NOT** use `HE_VERIFY` to validate data that comes from user input, data files, network, etc.

The `HE_VERIFY` macro also returns the boolean result of the condition statement (first parameter), and commonly you'll want to use it in a conditional to handle the failure case.

Example:

```cpp
void BoostShip(int32_t boost)
{
    if (!HE_VERIFY(boost > 0,
        HE_MSG("Boost value must be larger than zero."),
        HE_KV(boost, boost)))
    {
        return;
    }

    DoBoostStuff(boost);
}
```

### When to use `HE_LOG_ERROR`

The logging system should be used anytime there is unexpected behavior or data input to leave a record of this misbehavior.

## Appendix

### Why avoid STL headers?

There are two major reasons to avoid STL headers:

1. STL headers can have a big impact on compilation times
2. STL implementations vary by platform & compiler

The STL is very large and is intended to solve many use-cases. Unfortunately, due to the way that templates and `#include` works in C++ including any STL header has a chance of triggering long compile times. When included into engine headers used throughout projects, this can add up to a lot of time. Harvest has lightweight alternatives to many STL headers in the core library that should be preferred where possible.

Harvest Engine is a cross-platform engine and tries to offer a guarantee that if your implementation works _and is performant_ on one platform, it will work and be performant on all of them. Ensuring consistent performance across various platforms and compilers is very difficult with the STL, because there are many different implementations that make different tradeoffs. Harvest makes a consistent tradeoff choice that will improve consistency of behavior and performance across all supported platforms.

### Discouraged Features

#### Auto

While not "disallowed" use of `auto` is discouraged. A few cases where it is acceptable are:

- Variables that hold a lambda, as it is required
- Container iterators, as long as the type is made immediately obvious
- Template metaprogramming where types can be very complex and distract from logic

Examples:

```cpp
// OK, auto is required here
auto func = []() {};

// OK, element type is made obvious immediately and iterator types are complex and distracting
auto it = container.find(key);
const MyClass& element = *it;

// OK, element type is made obvious immediately in loop body
for (auto&& it : container)
{
    const MyClass& element = *it;
}

// Discouraged, explicitly specify the type
auto c = static_cast<const char*>(x);

// Discouraged, have to inspect the function to know the return type
auto r = DoStuff();
```

#### Goto

Goto usage should be avoided. In cases where there is cleanup to perform at multiple function exists utilize `he/core/scope_guard.h` utilities instead.

#### Exceptions

Exceptions (throw/try/catch) should be avoided. By default, stack unwinding is disabled by compiler flags.

Structured Exception Handling (SEH) is allowed, but should be used sparingly and only when writing platform-specific code on a platform that uses SEH.

#### RTTI

Run time type information should avoided. By default, RTTI is disabled by compiler flags. If reflection data is required then use one of the utilities in the `he\core\type_info.h` file or use the Harvest Schema IDL for detailed reflection.

#### Coroutines

Coroutines should be avoided.

TODO: Expand here about the performance costs, confusing nature of synchronization primitives, etc.

#### User-defined Literals

User-defined literals should be avoided. The types they create are not obvious, and non-standard literals may decrease reading comprehension.

#### Class Template Argument Deduction (CTAD)

Class template argument deduction (CTAD) should be avoided. Understanding that a type is a template, and what template arguments it uses can be difficult to understand when they are omitted. Be explicit about template types where possible.
