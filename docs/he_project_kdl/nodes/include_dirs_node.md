# `include_dirs` node

A set of directory paths to search for includes.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

- `external` (boolean) - Optional. When set to `#true` the path is treated as "external". The behavior for external files is configured using the [`external`](external_node.md) node. The default is `#false`.

## Children

- `directory-path` - Required. The [path](../paths.md) to a directory to add to the include path.
    * Directory paths may also specify a boolean property: `external`, which will override the `include_dirs` node's `external` property. The default is `#false`.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)
- [`public`](public_node.md)

## Example

```kdl
include_dirs { "include" }
```
