# `module` node

A definition of a module provided by a plugin.

## Arguments

1. (string) - Required. Name of the module. Convention is to use `lower_snake_case`.

## Properties

- `kind` (string) - Required. The kind of module to be defined. Valid values are:
    * `app_console` - Code built as a console application (exe).
    * `app_windowed` - Code built as a windowed application (exe).
    * `lib_header` - Header-only module that does not generate any symbols to be linked.
    * `lib_static` - Code built as a static library (lib/a).
    * `lib_shared` - Code built as a shared library (dll/so).
    * `content` - Assets, configuration, and other content to be edited.
    * `custom` - Utility project which contains only custom build scripts.
    * `hemake_extension` - HE Make extension module used to build and load a `.csproj` into the HE Make CLI at runtime.
- `group` (string) - Optional. Name of the group this module belongs to. This will be a virtual folder in the workspace tree, and can include folder separators (`/`).
- `language` (string) - Optional. Language of the source to be compiled in the module project. If omitted, HE Make first tries to infer the language from `project_file`; otherwise it defaults to `cpp`. Valid values are:
    * `c` - C
    * `cpp` - C++
    * `csharp` - C#
- `project_file` (string) - Optional. Path to an external project file to use instead of generating a project file. `.csproj` is supported for normal external C# projects and for `hemake_extension` modules. When `language` is omitted, `.csproj` implies `csharp`.
- `entrypoint` (string) - Optional. Name of the application's entrypoint symbol.
- `target_name` (string) Optional. The name of the output target file. When not specified, the module's name is used.
- `target_extension` (string) Optional. The extension of the output target file. By default the extension used is defined by the toolset.
- `target_dir` (string) Optional. The directory for the output target file. When not specified, the default output directory is derived from [`build_output`](build_output_node.md) and the module kind.
- `make_import_lib` (boolean) - Optional. `#false` to prevent generation of an import library for Windows DLLs. Default: `#true`.
- `make_exe_manifest` (boolean) - Optional. `#false` to prevent generation of a manifest for Windows executables and DLLs. Default: `#true`.
- `make_map_file` (boolean) - Optional. `#true` to enable generation of a mapfile for Windows targets. Default: `#false`.

## Children

- [`build_event`](build_event_node.md)
- [`build_options`](build_options_node.md)
- [`build_rule`](build_rule_node.md)
- [`codegen`](codegen_node.md)
- [`defines`](defines_node.md)
- [`dependencies`](dependencies_node.md)
- [`dialect`](dialect_node.md)
- [`exceptions`](exceptions_node.md)
- [`files`](files_node.md)
- [`floating_point`](floating_point_node.md)
- [`import`](import_node.md)
- [`include_dirs`](include_dirs_node.md)
- [`lib_dirs`](lib_dirs_node.md)
- [`link_options`](link_options_node.md)
- [`optimize`](optimize_node.md)
- [`public`](public_node.md)
- [`runtime`](runtime_node.md)
- [`sanitize`](sanitize_node.md)
- [`symbols`](symbols_node.md)
- [`tags`](tags_node.md)
- [`bin2c_compile`](bin2c_compile_node.md) - Available when the HE Make bin2c extension is loaded.
- [`schema_compile`](schema_compile_node.md) - Available when the HE Make schema extension is loaded.
- [`shader_compile`](shader_compile_node.md) - Available when the HE Make shader extension is loaded.
- [`warnings`](warnings_node.md)
- [`when`](when_node.md)

## Scopes

- [`plugin`](plugin_node.md)

## Example

```kdl
module he_core kind=lib_static group="engine/libs" {
    files { "include/**"; "src/**"; "debugger/**" }
    include_dirs { "src" }

    public {
        include_dirs { "include" }
    }
}
```
