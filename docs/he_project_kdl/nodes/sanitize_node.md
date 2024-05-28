# `sanitize` node

Sets which sanitizers to enable in the compiler toolset.

## Arguments

None.

## Properties

- `address` (boolean) - Optional. Enables Address Sanitizer (ASan) support.
- `fuzzer` (boolean) - Optional. Enables LibFuzzer support.
- `thread` (boolean) - Optional. Enables Thread Sanitizer (TSan) support.
- `undefined` (boolean) - Optional. Enables Undefined Behavior Sanitizer (UBSan) support.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
sanitize address=#true
```
