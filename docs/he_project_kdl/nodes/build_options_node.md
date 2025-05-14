# `build_options` node

A set of options that configure how a module is built.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

- `clr` (string) - Optional. Enable or disable Visual Studio Common Language Runtime (CLR) compilation. Specifying `off` for a `csharp` project produces an error. Visual Studio only.
    * `on` - Enable CLR support.
    * `off` - Disable CLR support. This is the default behavior.
    * `netcore` - Enable CLR support and output metadata for .NET Core.
- `mfc` (string) - Optional. Enable of disable Microsoft Foundation Class (MFC) Library support. Visual Studio only.
    * `off` - Disable MFC support. This is the default behavior.
    * `static` - Enable static linking of MFC libraries.
    * `dynamic` - Enable dynamic linking of MFC libraries.
- `atl` (string) - Optional. Enable Active Template Library (ATL) support. Visual Studio only.
    * `off` - Disable ATL support. This is the default behavior.
    * `static` - Enable static linking of ATL libraries.
    * `dynamic` - Enable dynamic linking of ATL libraries.
- `dpiawareness` (string) - Optional. Enable DPI Awareness for the application. Visual Studio only.
    * `none` - No special DPI Awareness enabled. This is the default behavior.
    * `high` - The application is high DPI aware.
    * `high_permonitor` - The application is per-monitor high DPI aware.
- `pch_include` (string) - Optional. The name of the precompiled header. This string is the name source files should use to include the header, not the path to file.
- `pch_source` (string) - Optional. The path to a source file which triggers the compilation of the header.
- `rtti` (boolean) - Optional. `#true` to enable runtime type information. Default: `#false`.
- `run_code_analysis` (boolean) - Optional. `#true` to enable running code analysis during build for Visual Studio projects. Default: `#false`.
- `run_clang_tidy` (boolean) - Optional. `#true` to enable running clang tidy code analysis during build for Visual Studio projects. Default: `#false`.
- `omit_default_lib` (boolean) - Optional. `#true` to omit the default runtime library name from the generated `.obj` file. Default: `#false`.
- `omit_frame_pointers` (boolean) - Optional. `#true` to suppress creation of frame pointers on the call stack. Default: `#false`.
- `openmp` (boolean) - Optional. `#true` to enable OpenMP support. Default: `#false`.

## Children

- `option` - Optional. Build option strings to pass directly to the compiler.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
build_options { "-Wpedantic"; "-Wno-static-in-inline" }
```
