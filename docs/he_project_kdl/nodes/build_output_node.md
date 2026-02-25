# `build_output` node

Defines the output directories used during build operations.

## Arguments

None.

## Properties

- `bin_dir` (string) Optional. Destination for application and shared modules binaries.
    * Default is: `${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/bin`.
- `gen_dir` (string) Optional. Destination for generated files.
    * Default is: `${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/generated`.
- `lib_dir` (string) Optional. Destination for static library modules.
    * Default is: `${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/lib`.
- `obj_dir` (string) Optional. Destination for object and intermediate files.
    * Default is: `${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/obj`.

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
build_output my_build_dir
```
