# `build_options` node

A set of options that are passed to the compiler directly.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `match` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `option` - Required. The build option string to pass to the compiler.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
build_options { "-Wpedantic"; "-Wno-static-in-inline" }
```
