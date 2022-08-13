# Overview

## Purpose

TODO:
- Why this document exists

## How to Read

TODO:
- Where to start?
- What do the sections mean?

# How To Contribute

## Code of Conduct

Read and understand the [Code of Conduct](CODE_OF_CONDUCT.md), it governs all development of the project.

## Reporting an Issue

TODO

## Submitting Code

TODO

### Agreement

TODO:
- Agree to Code of Conduct
- Transfer of copyright for submitted code

## Asking for Help

TODO

# Design Principles

## Keep it Consistent

TODO:
- When in Rome is the golden rule.

## Keep it Simple

TODO:
- Prefer simple types (`const char*`) where able.
- Additional complexity should be justified by a well-defined use-case.
- Code should be easy to change.

## Keep it Explicit

TODO:
- Explicit is better than implicit.
- Not everyone knows everything, help them out.

## Keep it Performant

TODO:
- Performance is important, keep it top of mind when doing anything.
- Sometimes there are tradeoffs to consider, and performance doesn't need to be #1 goal all the time.

## Keep it Extensible

TODO:
- Everything is a module in a plugin.
- Think of how other modules can extend your functionality
    * For example, core abstracts platforms; how does another plugin extend platform support?
    * For example, editor has a lot of UI; how does another plugin extend that UI?

## Keep it Readable

# Code Organization

## Naming

TODO:
- File names should be in `lower_snake_case` and match the name of the primary class they declare
    * If no primary class, a reasonable name for the collection of utilities or types
    * E.g.: `string.h` declares `he::String`
    * E.g.: `file_loader.h` declares `he::FileLoader`
- Headers should end with `.h`
- Source files should end with `.cpp`
- Inline implementation files should end with `.inl`
- Program entry-points should be in a `<name>_main.cpp` file that has no header.
    * Omit the `he_` prefix for Harvest modules
    * E.g.: module `he_bin2c` uses `bin2c_main.cpp`
- A module entry-point should be in a `<name>_module.cpp` file that has no header.
    * Omit the `he_` prefix for Harvest modules
    * E.g.: module `he_assets_editor` uses `assets_editor_module.cpp`
- Unit tests are in files named `test_<name>.cpp` that has no header.
    * `<name>` should be the name of the file being tested (without extension)
    * E.g.: file `he/core/string.h` is tested by `test_string.cpp`
- Unit test names should follow the following format:
    * Module name: name of the module without `he_` prefix
    * Suite name: name of the file being tested (without extension)
    * Test name: name of the class function being tested, or a name of the flow being tested
    * E.g.: When testing `he::String::Append()` from `he/core/string.h` the name should be `HE_TEST(core, string, Append)`

## Source Files

TODO:
- Code is written in C++20
- Avoid non-standard extensions
- Avoid [Discouraged Features](#bad-features)
- Header files should end in `.h`
- Source files should end in `.cpp`
- Inline source (non-api code) should end in `.inl`
- All files should start with a copyright header

### Header Files

TODO:
- Start with copyright header & `#pragma once`
- Forward declare when possible to reduce include tree depth
    * Exception: Don't need to forward declare templates, unless they are very simple (like `std::hash` for example)
-

In general, every `.cpp` file should have an associated `.h` file. There are a few notable exceptions to this rule:

- A program entry-point is often a `main.cpp` file that has no header.
- A module entry-point is often a `*_module.cpp` file that has no header.
- Unit tests are in files named `test_*.cpp` that has no header.

## Plugins & Modules

TODO:
- Everything is in a module within a plugin
- When should Module functionality should be exposed via RegisterApi?
    * When should I include a header and use something vs using RegisterApi?
    * Is it only for dynamic/optional dependencies?
    * Registered apis are singletons.
    * Registered apis are IoC, so plugins can be optionally loaded to provide them.
    * RegisterApi works cross-dll without exporting, includes don't.
- File layout for plugin with one module:
    * `plugin_name/he_plugin.toml`
    * `plugin_name/include/module_name/`
    * `plugin_name/src/`
    * `plugin_name/test/`
- File layout for plugin with multiple module:
    * `plugin_name/he_plugin.toml`
    * `plugin_name/module_name1/include/module_name1/`
    * `plugin_name/module_name1/src/`
    * `plugin_name/module_name1/test/`
    * `plugin_name/module_name2/include/module_name2/`
    * `plugin_name/module_name2/src/`
    * `plugin_name/module_name2/test/`

# API Design

## Module Names

TODO:
- Modules provided with the engine should be prefixed with `he_`
    * E.g.: `he_core`, `he_schema`, `he_editor`

## Namespaces

TODO:
- Should generally follow module naming.
    * `he_schema` uses `he::schema`
    * `he_assets` uses `he::assets`
    * Except `he_core` which claimed root `he`

## Basic Types

TODO:
- Use fixed-width integer values (`uint8_t`, `int32_t`)
- Prefer unsigned over signed when the value should never be negative
    * Small wrapping burden here, but worth expressing intent
- Use `uint8_t` to represent bytes/blobs (`uint8_t[]`)
- Use `void*` as a last resort

## Strings

TODO:
- Strings should be `char` sequences (not `wchar_t`)
- All strings should be UTF-8 encoded

## Classes & Structs

TODO:
- Structs should be [Aggregates](https://en.cppreference.com/w/cpp/language/aggregate_initialization)
    * No methods, or very few (like operators), no polymorphism
- Classes can be any functionality, but if its POD use Struct

## Parameter Ordering

TODO:
- Destination (or out param) first or last?
    * I'm inconsistent currently

## Use Hard Types

TODO:
- Prefer hard types (`struct Id{ uint32_t val; };`) over typedefs (`using Id = uint32_t;`)
- Always use scoped enums (`enum class`), not unscoped enums (`enum`)

## Thread-Safety

TODO:
- Functions are assumed to be not thread-safe, unless otherwise documented.
- Objects are summed to be not thread-safe, unless otherwise documented.

## Error Handling

### Exposing Errors

Errors should be expressed through a boolean or enum return value. Use a bool unless the type of error would influence the behavior of a caller, in which case use an enum.

For platform abstractions where the implementation will be interfacing with the Operating System use the `he::Result` structure which represents an operating system result code.

### When to use HE_ASSERT

The `HE_ASSERT` macro is used to validate preconditions that MUST be true. That is, without the condition being true the program must immediately halt either because continuing would crash anyway or because continuing would cause data corruption. Generally speaking assertions should be used to validate *programmer* assumptions, not *user* assumptions or data.

- DO use `HE_ASSERT` to validate array bounds access and other memory overwrite errors.
- DO NOT use `HE_ASSERT` to validate function parameters are within expected values (use `HE_VERIFY` instead).
- DO NOT use `HE_ASSERT` to validate data that can comes from user input, data files, network, etc.

`HE_ASSERT` is compiled out of the program when `HE_ENABLE_ASSERTIONS=0`. By default, the Shipping configuration will define `HE_ENABLE_ASSERTIONS=0` and therefore strip assertions from the build.

### When to use HE_VERIFY

The `HE_VERIFY` macro is used to validate preconditions that SHOULD be true. That is, a bug is present if the condition is not true but the program can recover. Most commonly `HE_VERIFY` is used to validate expected state or input which when incorrect can lead to bad behavior. However since the program can continue, it does so after logging the issue.

- DO use `HE_VERIFY` to validate function parameters are within expected values.
- DO NOT use `HE_VERIFY` to validate data that comes from user input, data files, network, etc.

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

### When to use HE_LOG_ERROR

The logging system should be used anytime there is unexpected behavior or data input to leave a record of this misbehavior.

# Code Style Guidelines

## EditorConfig

TODO:
- Whatever editor you use should utilize EditorConfig
- Detailed formatting options for Visual Studio are in the `.editorconfig` file

## Whitespace & Braces

TODO:
- All indentation should use 4 spaces. No tab characters.
- Allman style braces (on their own line)
- Alignment of comments in various places
- Spaces for parenthesis stuff (no space)
- Spaces for aggregate initializers (leading/trailing space)
- Operators always have a space on either side
    * Don't rely on operator precedence. Always use parenthesis when using multiple operators

## Line Length

TODO:
- Comments have a hard length max at 100
- Code has a soft length max at 150

## Comments

TODO:
- Prefer single-line comments (`//`) to multi-line comments (`/* */`)
- Use documentation comments (`///`) for doxygen comments
    * Examples of various doxygen stuff
- Use comments to explain "why" rather than "what", unless the code/flow is non-obvious
- When to use separator comments (`// -------------`) that end at col 100

## Braces

TODO:
- Allman style braces (on their own line)

## Naming

TODO:
- No Hungarian notation
- lowerCamelCase for variables
- UpperCamelCase for classes, enums, structs, functions, and compile-time constants
- lower_snake_case for namespace names, and key names (`HE_KV`)
- UPPER_SNAKE_CASE for preprocessor macros
    * All macros should be prefixed with `HE_`

## Namespaces

TODO:
- Use C++17 nested namespace definitions (`namespace a::b {}`) instead of nesting namespace blocks (`namespace a { namespace b {} }`)
    * If you must use a nested namespace block, do not indent it
- Generally all constructs should be within a namespace
- Code within a namespace should be indented by one.

## Conditionals & Loops

TODO:
- Omit braces if the condition is simple, and the statement is simple
    * Macros must always be within braces
    * Comments count towards requiring braces
- Add braces if there are `else` statements that follow
- If any if in an if-else chain has braces, they all must have braces
- Multiline conditionals should lead with the operator (i.e.: start with `&&` or `||`)
- Loops should always have braces, even when there is only a single line of body

## Type aliases

TODO:
- Use `using` syntax instead of `typedef`

## Preprocessor Directives

TODO:
- Indent them like normal code
- Start indentation one indent left of surrounding code
- Macros with multiple statements should be encoded in `do { } while (0)`
    * Exception: if the macro needs to return the value of the statemenets.
- Function-like macros should not end in a semicolon, so the caller of a function-like macro can specify one
- Prefer functions and templates to function-like macros
- Try to avoid using conditional compilation where possible
    * For platform-specific code use file suffixes (`*.win32.cpp`, `*.posix.cpp`, etc) and minimal `#if` branching
    * Hide compiler branching behind a macro definition (conditionally define the macro so users don't branch)
- Use `#if defined()` instead of `#ifdef` and `#if !defined()` instead of `#ifndef`
    * Avoid conditionally defining macros, prefer defining them as `0` or `1` instead
    * Exception is defines from premake scripts that are extensible
        - Example: `HE_PLATFORM` APIs are conditionally defined because plugins can add new ones

### Fully Qualified Names

All generated code, including code generated by macros, should use fully qualified names. This includes the global `::` prefix.

Macros can be used in any namespace context, so always using the fully qualified name for types ensures the macro works in any context.

Example:

```cpp
#define HE_MAKE_STRING(name, value) ::he::String name(#value)
```

## Includes

TODO:
- All includes, including from contrib packages, should use quotes (`""`), except for system includes (such as STL) which use angle brackets (`<>`).
- Include what you use, don't rely on transient includes.
- Forward declare when possible to reduce include tree depth
    * Exception: Don't need to forward declare templates, unless they are very simple (like `std::hash` for example)

### Grouping

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

#include "fmt/core.h"

#include <new>
```

## Switch-Case

TODO:
- indent case statements
- Add a block if you declare any variables, or if the logic is more complex than a few simple statements.

## Const-correctness

TODO:
- Rule of thumb: use `const` unless you can't
    * Exception: Pointers to const-data is acceptable. E.g: `const char* A;` is OK, don't need `const char* const A;`
    * Exception: Pass-by-value parameters don't need to be marked `const`
- Same goes for methods
- For pointers place the `const` before the `*`: `const char*`
    * Do not do `char const*`

## Initialize Variables

TODO:
- Always initialize block-scoped variables, even if you pass it somewhere immediately for writing.
    * Default initialization is OK for 'expensive' to initialize types

## Internal Linkage

TODO:
- Prefer constructs declared with `static` and types with a leading underscore in the name (`_`) to anon namespaces

## Functions

TODO:
- Default arguments are OK
- Lambdas are OK, but generally free functions should be preferred
    * Simple iterators or predicates that are immediately passed into a function are OK
- Avoid function pointer parameters, instead prefer `he::Delegate`
    * Templates for callbacks are OK
- All params on one line, or all params separated by newlines with one indent
    * Do not split params across multiple lines unless you split *all* params on multiple lines
    * Same for function calls, all on one line or all on separate lines
- Omitting unused parameter names when their use is obvious (like a deleted copy ctor)
    * Otherwise write the name and use `HE_UNUSED` to avoid warnings if needed

## Casting

TODO:
- Prefer C++-style casting in all cases
    * Do not use `dynamic_cast`, RTTI is disallowed
    * Do not use c-style casts (`(int)x`) or constructor-style casts (`int(x)`)
    * Use `static_cast` unless you can't

## Null

TODO:
- Use `nullptr` for null pointers instead of `NULL` or `0`
- Use `'\0'` for the null character instead of `0`


## Classes & Structs

TODO:
- Always mark constructors and destructors as `noexcept`
    * Exceptions are not supported in Harvest
    * By default it is implicit based on constructed/destructed types. Being explicit can catch cases where we have a type that can except, and we don't want it to.
    * Also do it for `= default;` declaration.
- Use inline initializers for members, using universal initialization syntax (`{}`).
    * Exception: When a member is initialized by a constructor parameter
    * Exception: When a struct is meant to be value or default initialized, do not specify initializers (like `Vec2`)
- Do not call virtual functions from constructors
- Use `explicit` for constructors with a single parameter
    * Exception: copy/move ctors
- Follow the [Rule of Five](https://en.cppreference.com/w/cpp/language/rule_of_three) for constructors
    * Polymorphic classes should disable copy semantics
- Make members private by default, and take care for exposed data
    * Managing invariant assumptions is easier for private data
    * Also use access specifiers to group similar things together in this order (all this public, then protected, then private):
        1. Types and type aliases (`using`, `enum class`, `struct`, etc)
        2. Static constants
        3. Static functions
        4. Constructors and operators
        5. Destructor
        6. Non-static member functions
        7. Data members

### Getters

TODO: review for correctness

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

### Setters

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

## <a name="bad-features"></a>Discouraged Features

### Auto

While not "disallowed" use of `auto` is discouraged. A couple cases where it is acceptable are:

- Variables that hold a lambda (where it is required)
- Iterators in range-based for loops (as long as it is typed immediate)
- Template metaprogramming where types can be very complex and distract from logic

Examples:

```cpp
// OK, auto is required here
auto func = []() {};

// OK, element type is made obvious immediately and iterator types are complex and distracting
auto it = container.find(key);
const ElementType& element = *it;

// OK, element type is made obvious immediately in loop body
for (auto&& it : container)
{
    const ElementType& element = *it;
}

// Discouraged, specify the type as normal
auto c = static_cast<const char*>(x);

// Discouraged, have to inspect the function to know the return type
auto r = DoStuff();
```

### Goto

Goto usage should be avoided in almost all cases. In cases where there is cleanup to perform at multiple function exists utilize `he/core/scope_guard.h` utilities instead.

### Exceptions

TODO: expand on this text, what about SEH?

Exceptions (throw/try/catch) should not be used at any time. Stack unwinding is disabled by compiler flags.

### RTTI

TODO: expand on this text

Run time type information should not be used at any time, and is disabled by compiler flags. If reflection data is required for logic then utilize the Harvest Schema IDL to generate such information at build time.

### Coroutines

TODO:
- Don't use them and stuff

### User-defined Literals

TODO:
- Types are not obvious, avoid them

### Class Template Argument Deduction (CTAD)

TODO:
- Makes types more obscure, avoid use

# Git Workflow

## Trunk-based Development

TODO:
- Branching Strategy
    * `main` is the latest development branch.
    * `release/x.y` is the release branch for a particular major/minor release.
    * `feature/a` is for *short-lived* feature branches intended to merge into `main` via PR.
    * `bugfix/b` is for *short-lived* bug fixes intended to merge into `main` via PR.
    * No committing directly to `main`.
    * No long-lived `feature/` or `bugfix/` branches, delete them once they merge to `main`.
    * Any merged into a `release/` branch should be merged to `main` as well, or implemented there separately if necessary.

## Prefer Rebasing

TODO:
- Try to keep the main line as straight as possible, no octopus merging
- Merge PRs with the `Squash and Merge` strategy and keep the PR number in the summary
- Default to rebase for `git pull` either by using the recommended config below or specifying `git pull --rebase`

## Commit Messages

TODO:
- First line is a summary and should be 50 characters or less
    * Do not use markdown in your summary line
- Separate details from the summary with a blank line
    * Details should be wrapped at 80 characters
    * Details can contain markdown formatting
- Content should be descriptive and augment what can be seen in code
    * Don't just mention which files were touched, describe why changes were made

Good guide: https://cbea.ms/git-commit/

Example:

```txt
This is the summary line, it is 50 characters long

This is the detail text. It can go up to eighty characters long before it
should be wrapped. Markdown `formatting` is also reasonable here.

- Bullets and such
- Are totally fine
```

## Recommended Configuration

Here is a configuration file that is recommended when you get started. Remember to replace the `[user]` block with your information:

```ini
[user]
	name = Your Name
	email = your_email@domain.com
[init]
	defaultBranch = main
[pull]
	rebase = true
[fetch]
	prune = true
[color]
	ui = auto
```

### Useful Aliases

Here are some aliases that can be useful when working with command-line git:

```ini
[alias]
	co = checkout
	ls = status
	lol = log --graph --decorate --pretty=oneline --abbrev-commit --date=relative --format=format:'%C(red)%h%C(reset) - %C(green)(%ar)%C(reset) %s - %C(dim cyan)%an%C(reset) %C(auto)%d%C(reset)'
	sha = rev-parse HEAD
	ha = rev-parse --short HEAD
	stat = show --stat=150
	gc-branches = "!LANG=en_US git branch -vv | awk '/: gone]/{print $1}' | xargs git branch -D"
```
