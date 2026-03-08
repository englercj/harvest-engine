# `schema_compile` node

High-level schema compilation for a module. This node is provided by the HE Make schema extension.

It expands into a generated companion module, custom build rules, and the appropriate owner-module dependency wiring.

## Arguments

- (string) - Required. One or more schema compiler targets, such as `c++`.

## Properties

- `scope` (string) - Optional. Whether the owner module depends on the generated companion module through `public` or private `dependencies`. Default: `public`.
- `group` (string) - Optional. Virtual solution folder for the generated companion module. Default: `_generated/schema`.

## Children

- [`files`](files_node.md) - Required. Schema source files to compile.
- [`include_dirs`](include_dirs_node.md) - Required. Include roots used both for schema imports and for determining generated output layout.
- [`dependencies`](dependencies_node.md) - Optional. Names of source modules that also expose `schema_compile` nodes.

## Behavior

- Generates a companion module named `<owner>__schemac_<n>` with kind `lib_static`.
- Adds a dependency from the owner module to the companion module according to `scope`.
- Adds generated outputs under `${module[<owner>].gen_dir}/include/...`.
- Adds companion-module dependencies on `he_core`, `he_schema`, `he_schemac` (`kind=order`), and generated schema companions for declared `dependencies`.
- `dependencies` must name source modules that declare `schema_compile`; naming generated modules directly is an error.

## Scopes

- [`module`](module_node.md)

## Example

```kdl
schema_compile "c++" scope=public {
    files { "include/he/example/data.hsc" }
    include_dirs { "include" }
    dependencies { other_module }
}
```
