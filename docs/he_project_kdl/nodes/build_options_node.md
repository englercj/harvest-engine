# `build_options` node

A set of options that configure how a module is built.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

- `clr` (string) - Optional. Enable or disable Visual Studio Common Language Runtime (CLR) compilation. Specifying `off` for a `csharp` project produces an error.
    * `on` - Enable CLR support.
    * `off` - Disable CLR support. This is the default behavior.
    * `netcore` - Enable CLR support and output metadata for .NET Core.
- `mfc` (string) - Optional. Enable of disable Microsoft Foundation Class (MFC) Library support.
    * `off` - Disable MFC support. This is the default behavior.
    * `static` - Enable static linking of MFC libraries.
    * `dynamic` - Enable dynamic linking of MFC libraries.
- `atl` (string) - Optional. Enable Active Template Library (ATL) support.
    * `off` - Disable ATL support. This is the default behavior.
    * `static` - Enable static linking of ATL libraries.
    * `dynamic` - Enable dynamic linking of ATL libraries.
- `run_code_analysis` (boolean) - Optional. `#true` to enable running code analysis during build for Visual Studio projects. Default: `#false`.
- `run_clang_tidy` (boolean) - Optional. `#true` to enable running clang tidy code analysis during build for Visual Studio projects. Default: `#false`.

## Children

- `option` - Optional. Build option strings to pass directly to the compiler.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
build_options { "-Wpedantic"; "-Wno-static-in-inline" }
```
