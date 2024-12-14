# `build_output` node

Defines the output directories used during build. This is usually specified in a [`project`](project_node.md) node, but can also be overridden within individual [`module`](module_node.md) nodes.

If not specified the build output directory will be a directory named `build` in the directory where hemake is invoked.

## Arguments

1. (string) - Required. The base directory for output files, relative to the project's file.

## Properties

All of these directory properties are relative to the base directory specified in the first argument.

- `bin_dir` (string) Optional. Destination for application and shared modules binaries.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/bin`.
- `gen_dir` (string) Optional. Destination for generated files.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/generated/${module.name}`.
- `lib_dir` (string) Optional. Destination for static library modules.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/lib/${module.name}`.
- `obj_dir` (string) Optional. Destination for object and intermediate files.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/obj/${module.name}`.
- `plugin_dir` (string) Optional. Destination for install plugins.
    * Default is: `plugins`.
- `project_dir` (string) Optional. Destination for generated build system project files.
    * Default is: `projects`.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
build_options { "-Wpedantic", "-Wno-static-in-inline" }
```
