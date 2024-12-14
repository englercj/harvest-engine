# `defines` node

A set of preprocessor symbols to be defined.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `symbol` - Required. The symbol to define.

## Scopes

- [`module`](module_node.md)
- [`private`](private_node.md)
- [`project`](project_node.md)
- [`public`](public_node.md)

## Example

```kdl
defines { HE_PLATFORM_WIN64 }

defines { "HE_IMPL_PLATFORM_RWLOCK_SIZE=56" }
```
