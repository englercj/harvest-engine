# `link_options` node

A set of options that configure how a module is linked.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

- `incremental` (boolean) - Optional. `#false` to disable incremental linking. Default: `#true`.

## Children

- `option` - Optional. Linker option strings to pass directly to the linker.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
link_options { "-Wl,--fatal-warnings" }
```
