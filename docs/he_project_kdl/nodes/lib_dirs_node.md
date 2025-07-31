# `lib_dirs` node

A set of directory paths to search for libraries.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add matched items to the set, or update it if it already exists. This is the default behavior.
    * `remove` - Remove matched the items from the set.
    * `update` - Update properties of matched items if they exist in the set.

## Properties

- `system` (boolean) - Optional. When set to `#true` the path is used to search for system libraries. The default is `#false`.

## Children

- `path` - Required. The [path](../paths.md) to a directory to add to the include path.
    * Paths may also specify an `system` property to override the `lib_dirs` node's `system` property.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)
- [`public`](public_node.md)

## Example

```kdl
lib_dirs { "lib" }
```
