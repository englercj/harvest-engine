# `toolset` node

Configures how a compiler toolset is used in the project. Can configure multiple compiler toolsets by repeating this node with a different argument.

Note that this node does not set which toolset is used for compilation. That is handled by the [`platform`](platform_node.md) node.

## Arguments

1. (string) - Required. Which set of build tools is being configured. Valid values are:
    * `clang` - C language frontend for LLVM
    * `gcc` - GNU Compiler Collection
    * `msvc` - Microsoft Visual C++ Compiler

## Properties

- `arch` (string) - Optional. The preferred architecture for build tools. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `x86` - Use 32-bit x86 build tools.
    * `x86_64` - Use 64-bit x86 build tools.
- `edit_and_continue` (boolean) Optional. `#true` to enable edit-and-continue. Default: `#false`.
- `fast_up_to_date_check` (boolean) - Optional. `#true` to enable MSVC's fast up-to-date checking. Default: `#true`.
- `log` (string) - Optional. Output path for build logs.
- `multiprocess` (boolean) - Optional. `#true` to enable multi-process compilation. Default: `#false`.
- `path` (string) - Optional. Explicit path to the toolset to be used.
- `version` (string) - Optional. The preferred version to use when multiple installations are available.
    * For `msvc` this is the three-component toolset version. For example: `14.39.33519`.

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
toolset msvc arch=x86_64
```
