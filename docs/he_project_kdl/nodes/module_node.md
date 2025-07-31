# `module` node

A definition of a module provided by a plugin.

## Arguments

1. (string) - Required. Name of the module. Used as the project name in generated build project files, so must be unique within a workspace.

## Properties

- `kind` (string) - Required. The kind of module to be defined. Valid values are:
    * `app_console` - Code built as a console application (exe).
    * `app_windowed` - Code built as a windowed application (exe).
    * `content` - Assets, configuration, and other content to be edited.
    * `custom` - Utility project which contains only custom build scripts.
    * `lib_header` - Header-only module that does not generate any symbols to be linked.
    * `lib_static` - Code built as a static library (lib/a).
    * `lib_shared` - Code built as a shared library (dll/so).
- `group` (string) - Optional. Name of the group this module belongs to. This will be a virtual folder in the workspace tree, and can include folder separators (`/`).
- `language` (string) - Optional. Language of the source to be compiled in the module project. Default is `cpp` if not specified. Valid values are:
    * `c` - C
    * `cpp` - C++
    * `csharp` - C#
- `project_file` (string) - Optional. Path to a project file to use instead of generating one.
- `entrypoint` (string) - Optional. Name of the application's entrypoint symbol.
- `hemake_extension` (boolean) - Optional. `#true` to tell HE Make to load this project as an extension. Default: `#false`

## Children

- [`build_event`](build_event_node.md)
- [`build_options`](build_options_node.md)
- [`build_output`](build_output_node.md)
- [`build_rule`](build_rule_node.md)
- [`codegen`](codegen_node.md)
- [`defines`](defines_node.md)
- [`dialect`](dialect_node.md)
- [`exceptions`](exceptions_node.md)
- [`external`](external_node.md)
- [`files`](files_node.md)
- [`floating_point`](floating_point_node.md)
- [`import`](import_node.md)
- [`link_options`](link_options_node.md)
- [`optimize`](optimize_node.md)
- [`public`](public_node.md)
- [`runtime`](runtime_node.md)
- [`sanitize`](sanitize_node.md)
- [`symbols`](symbols_node.md)
- [`tags`](tags_node.md)
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
