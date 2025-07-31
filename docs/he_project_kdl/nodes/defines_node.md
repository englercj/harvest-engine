# `defines` node

A set of preprocessor symbols to be defined.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add matched items to the set, or update it if it already exists. This is the default behavior.
    * `remove` - Remove matched the items from the set.
    * `update` - Update properties of matched items if they exist in the set.

## Properties

None.

## Children

- `symbol` - Required. The symbol to define.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)
- [`public`](public_node.md)

## Example

```kdl
defines { HE_PLATFORM_WIN64 }

defines { "HE_IMPL_PLATFORM_RWLOCK_SIZE=56" }
```
