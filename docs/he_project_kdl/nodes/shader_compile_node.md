# `shader_compile` node

High-level shader compilation for a module. This node is provided by the HE Make shader extension.

It expands into a generated companion module and custom build rules that invoke `he_shaderc`.

## Arguments

- (string) - Required. One or more shader targets, such as `sm_6_0` or `glsl_450`.

## Properties

- `scope` (string) - Optional. Whether the owner module depends on the generated companion module through `public` or private `dependencies`. Default: `public`.
- `group` (string) - Optional. Virtual solution folder for the generated companion module. Default: `_generated/shader`.
- `optimize` (string) - Optional. Optimization level passed to `he_shaderc`.

## Children

- [`files`](files_node.md) - Required. Shader source files to compile.
- [`include_dirs`](include_dirs_node.md) - Optional. Include roots passed to `he_shaderc` and mirrored under the module `gen_dir`.
- [`defines`](defines_node.md) - Optional. Preprocessor defines passed to `he_shaderc`.

## Behavior

- Generates a companion module named `<owner>__shaderc_<n>` with kind `custom`.
- Adds a dependency from the owner module to the companion module according to `scope`.
- Adds a companion-module dependency on `he_shaderc` (`kind=order`).
- Emits headers named `<input-file-base>.shaders.h` under `${module[<owner>].gen_dir}/...`.

## Scopes

- [`module`](module_node.md)

## Example

```kdl
shader_compile "sm_6_0" scope=private optimize="2" {
    files { "src/shaders/imgui.slang" }
    include_dirs { "src/shaders" }
    defines { HE_SHADER_DEBUG }
}
```
