# `project` node

The root node of a Harvest Project structure.

## Arguments

1. (string) - Required. The name of the Harvest project.

## Properties

- `start` (string) - Optional. The name of which module to set as the default startup module.
- `build_dir` (string) Optional. The base directory for all build files. Default value: `.build`
- `installs_dir` (string) Optional. Directory for installed plugins. Default is: `${project.build_dir}/installs`.
- `projects_dir` (string) Optional. Destination for build system project files. Default is: `${project.build_dir}/projects`.

## Children

- [`build_options`](build_options_node.md)
- [`build_output`](build_output_node.md)
- [`build_rule`](build_rule_node.md)
- [`codegen`](codegen_node.md)
- [`configuration`](configuration_node.md)
- [`defines`](defines_node.md)
- [`dialect`](dialect_node.md)
- [`exceptions`](exceptions_node.md)
- [`external`](external_node.md)
- [`files`](files_node.md)
- [`floating_point`](floating_point_node.md)
- [`import`](import_node.md)
- [`include_dirs`](include_dirs_node.md)
- [`lib_dirs`](lib_dirs_node.md)
- [`link_options`](link_options_node.md)
- [`optimize`](optimize_node.md)
- [`option`](option_node.md)
- [`platform`](platform_node.md)
- [`plugin`](plugin_node.md)
- [`runtime`](runtime_node.md)
- [`sanitize`](sanitize_node.md)
- [`symbols`](symbols_node.md)
- [`system`](system_node.md)
- [`tags`](tags_node.md)
- [`toolset`](toolset_node.md)
- [`warnings`](warnings_node.md)
- [`when`](when_node.md)

## Scopes

None.

## Example

```kdl
project "Harvest Engine" start="he_editor" {
    import "./he_plugin.kdl"
}
```
