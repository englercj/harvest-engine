# `optimize` node

Set the optimization level and related compiler settings.

## Arguments

1. (string) - Required. The optimization level to set. Valid values are:
    * `debug` - Disable all optimizations, but with debugger support passes.
    * `off` - Disable all optimization passes.
    * `balanced` - Enable balanced optimizations.
    * `size` - Optimize for smallest executable size.
    * `speed` - Optimize for fastest run speed.

## Properties

- `lto` (string) - Optional. Enable or disable link-time optimization. Valid values are:
    * `default` - Use the compiler's default behavior. This is the default value.
    * `off` - Disable link-time optimization.
    * `on` - Enable link-time optimization.
- `inlining` (string) - Optional. Modify the compiler's inlining behavior. Valid values are:
    * `default` - Use the compiler's default behavior. This is the default value.
    * `off` - Disable inlining.
    * `explicit` - Only inline functions explicitly specified as `inline`.
    * `on` - Allow the compiler to inline any function it chooses.

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
