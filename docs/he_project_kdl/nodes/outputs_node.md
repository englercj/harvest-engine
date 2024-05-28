# `outputs` node

A set of output file paths for a [`build_rule`](build_rule_node.md).

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `match` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `file-path` (string) - Required. The path to files that should be tracked as output for the build rule.

## Scopes

- [`build_rule`](build_rule_node.md)

## Example

```kdl
outputs {
    "${file.gen_dir}/${file.path:basename}.hsc.h"
    "${file.gen_dir}/${file.path:basename}.hsc.cpp"
}
```
