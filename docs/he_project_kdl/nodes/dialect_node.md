# `dialect` node

Sets the dialect of one or more languages.

## Arguments

None.

## Properties

- `cpp` (string) - Optional. The language version to compile C++ code as. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `c++98` - ISO C++98
    * `c++11` - ISO C++11
    * `c++14` - ISO C++14
    * `c++17` - ISO C++17
    * `c++20` - ISO C++20
- `c` (string) - Optional. The language version to compile C code as. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `c99` - ISO C99
    * `c11` - ISO C11
    * `c17` - ISO C11
- `csharp` (string) - Optional. The language version to compile C# code as. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `c#8` - C# 8
    * `c#9` - C# 9
    * `c#10` - C# 10
    * `c#11` - C# 11
    * `c#12` - C# 12

## Children

None.

## Scopes

- [`project`](project_node.md)
- [`module`](module_node.md)

## Example

```kdl
dialect cpp=c++20 c=c11 csharp=c#12
```
