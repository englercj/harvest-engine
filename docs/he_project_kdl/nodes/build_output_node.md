# `build_output` node

Defines the output directories used during build. This is usually specified in a [`project`](project_node.md) node, but can also be overridden within individual [`module`](module_node.md) nodes.

If not specified the build output directory will be a directory named `.build` in the directory where hemake is invoked.

## Arguments

1. (string) - Optional. The base directory for output files, relative to the project's file. Default value: `.build`

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
- `install_dir` (string) Optional. Destination for installed plugins.
    * Default is: `installs`.
- `project_dir` (string) Optional. Destination for generated build system project files.
    * Default is: `projects`.
- `target_name` (string) Optional. The name of the output target file. When not specified, the parent module's name is used.
- `target_extension` (string) Optional. The extension to append to the target name. By default the extension used is defined by the toolset.
- `make_import_lib` (boolean) - Optional. `#false` to prevent generation of an import library for Windows DLLs. Default: `#true`.
- `make_exe_manifest` (boolean) - Optional. `#false` to prevent generation of a manifest for Windows executables and DLLs. Default: `#true`.
- `make_map_file` (boolean) - Optional. `#true` to enable generation of a mapfile for Windows targets. Default: `#false`.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
build_output .build

module some_app {
    build_output target_name="myapp"
}
```
