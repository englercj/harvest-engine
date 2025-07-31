# `outputs` node

A set of output file paths for a [`build_rule`](build_rule_node.md).

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add matched items to the set, or update it if it already exists. This is the default behavior.
    * `remove` - Remove matched the items from the set.
    * `update` - Update properties of matched items if they exist in the set.

## Properties

None.

## Children

- `file-path` (string) - Required. The path to files that should be tracked as output for the build rule.

## Scopes

- [`build_event`](build_event_node.md)
- [`build_rule`](build_rule_node.md)

## Example

```kdl
outputs {
    "${file.gen_dir}/${file.path:basename}.hsc.h"
    "${file.gen_dir}/${file.path:basename}.hsc.cpp"
}
```
