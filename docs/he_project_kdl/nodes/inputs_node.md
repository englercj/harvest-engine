# `inputs` node

A set of input file paths for a [`build_rule`](build_rule_node.md).

The file the build rule is running for is always treated as an input and doesn't need to be explicitly added. That is, you don't need `inputs { "${file.path}" }`.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `file-path` (string) - Required. The path to files that should be tracked as input for the build rule.

## Scopes

- [`build_rule`](build_rule_node.md)

## Example

```kdl
inputs {
    // Add the schemac executable as as input so if it changes, this rule gets run again.
    // "${file.path}" is always treated as an input, no need to specify it here
    :module he_schemac {
        "${module.link_target}"
    }
}
```
