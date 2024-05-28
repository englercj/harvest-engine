# `floating_point` node

Specifies how the compiler treats floating-point expressions, optimizations, and exceptions.

## Arguments

1. (string) - Required. The mode for floating point expressions.
    * `default` - Use the toolset's default warning behavior. This is the behavior.
    * `fast` - Allows the compiler to generate code that improves performance at the expense of accuracy.
    * `strict` - Allows the compiler to generate code that improves accuracy at the expense of performance.

## Properties

- `exceptions` (boolean) - Optional. When true allows code generation that can generate floating-point exceptions. The default is `#false`.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
floating_point fast
```
