# `build_output` node

Defines the output directories used during build operations.

If not specified the build output directory will be a directory named `.build` relative to the root project file.

## Arguments

1. (string) - Optional. The base directory for output files. Default value: `.build`

## Properties

All of these directory properties are relative to the base directory specified in the first argument.

- `bin_dir` (string) Optional. Destination for application and shared modules binaries.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/bin`.
- `gen_dir` (string) Optional. Destination for generated files.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/generated`.
- `lib_dir` (string) Optional. Destination for static library modules.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/lib`.
- `obj_dir` (string) Optional. Destination for object and intermediate files.
    * Default is: `${platform.name:lower}-${configuration.name:lower}/obj`.
- `install_dir` (string) Optional. Destination for installed plugins.
    * Default is: `installs`.
- `project_dir` (string) Optional. Destination for generated build system project files.
    * Default is: `projects`.

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
build_output my_build_dir
```
