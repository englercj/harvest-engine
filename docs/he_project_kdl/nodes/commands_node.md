# `commands` node

A set of commands to run for a [`build_rule`](build_rule_node.md).

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `match` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `command-line` (string) - Required. The command to run. Use the `cmd` [Token Context](../tokens.md) when appropriate for portable commands. There can be multiple command strings.

## Scopes

- [`build_rule`](build_rule_node.md)

## Example

```kdl
commands { "echo ${file.path}" }
```
