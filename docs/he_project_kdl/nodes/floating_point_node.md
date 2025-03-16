# `floating_point` node

Specifies how the compiler treats floating-point expressions, optimizations, and exceptions.

## Arguments

1. (string) - Required. The mode for floating point expressions.
    * `default` - Use the toolset's default behavior. This is the default value.
    * `fast` - Allows the compiler to generate code that improves performance at the expense of accuracy.
    * `precise` - The compiler preserves the source expression ordering and rounding properties of floating-point code when it generates and optimizes object code for the target machine.
    * `strict` - Similar to `precise` but the program may also safely access or modify the floating-point environment at runtime. This is generally more expensive than `precise`.

## Properties

- `exceptions` (boolean) - Optional. When true allows code generation that can generate floating-point exceptions. The default is `#false`.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
floating_point precise
```
