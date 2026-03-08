# `bin2c_compile` node

High-level binary-to-C header generation for a module. This node is provided by the HE Make bin2c extension.

It expands into a generated companion module and custom build rules that invoke `he_bin2c`.

## Arguments

None.

## Properties

- `scope` (string) - Optional. Whether the owner module depends on the generated companion module through `public` or private `dependencies`. Default: `public`.
- `group` (string) - Optional. Virtual solution folder for the generated companion module. Default: `_generated/bin2c`.
- `text` (boolean) - Optional. `#true` to emit string data instead of a byte array. Default: `#false`.
- `compress` (boolean) - Optional. `#true` to compress the input with `stb_compress`. Default: `#false`.

## Children

- [`files`](files_node.md) - Required. Files to convert.
- [`include_dirs`](include_dirs_node.md) - Optional. Include roots that determine where generated headers are exposed under the module `gen_dir`.

## Behavior

- Generates a companion module named `<owner>__bin2c_<n>` with kind `custom`.
- Adds a dependency from the owner module to the companion module according to `scope`.
- Adds a companion-module dependency on `he_bin2c` (`kind=order`).
- Emits headers named `<input-file>.h` under `${module[<owner>].gen_dir}/...`.
- Uses variable names of the form `c_<file-name-with-separators-replaced-by-underscores>`.

## Scopes

- [`module`](module_node.md)

## Example

```kdl
bin2c_compile scope=private compress=#true {
    files { "assets/logo.png" }
    include_dirs { "assets" }
}
```
