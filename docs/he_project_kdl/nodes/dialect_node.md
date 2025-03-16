# `dialect` node

Sets the dialect of one or more languages.

## Arguments

None.

## Properties

- `cpp` (string) - Optional. The language version to compile C++ code as. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `c++14` - ISO C++14
    * `c++17` - ISO C++17
    * `c++20` - ISO C++20
    * `c++23` - ISO C++23
- `c` (string) - Optional. The language version to compile C code as. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `c11` - ISO C11
    * `c17` - ISO C17
    * `c23` - ISO C23
- `csharp` (string) - Optional. The language version to compile C# code as. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `c#12` - C# 12
    * `c#13` - C# 13

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
dialect cpp=c++20 c=c11 csharp=c#12
```
