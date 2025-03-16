# `optimize` node

Set the optimization level and related compiler settings.

## Arguments

1. (string) - Required. The optimization level to set. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `debug` - Disable all optimizations, but with some debugger support.
    * `off` - Disable all optimization passes.
    * `on` - Enable a balanced set of optimization passes.
    * `size` - Enable optimizations for smallest executable size.
    * `speed` - Enable optimizations for fastest run speed.

## Properties

- `lto` (string) - Optional. Enable or disable link-time optimization. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `off` - Disable link-time optimization.
    * `on` - Enable link-time optimization.
- `inlining` (string) - Optional. Modify the toolset's inlining behavior. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `off` - Disable inlining.
    * `explicit` - Only inline functions explicitly specified as `inline`.
    * `on` - Allow the toolset to inline any function it chooses.
- `function_level_linking` (boolean) - Optional. `#true` to enable function level linking for Visual Studio projects. Default: `#true` for optimized builds.
- `string_pooling` (boolean) - Optional. `#true` to enable string pooling. Default: `#true` for optimized builds.
- `intrinsics` (boolean) - Optional. `#true` to allow the toolset to replace some functions with intrinsics or special function forms to help the application run faster. Default: `#true` for optimized builds.
- `just_my_code` (boolean) - Optional. `#true` to enable JustMyCode debugging support in Visual Studio projects. Default: `#true`.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
configuration Debug {
    optimize off lto=off inlining=off
}

configuration Release {
    optimize speed lto=on inlining=on
}
```
