# `tags` node

A set of tags to activate in the scope. Tags don't modify any behavior by themselves, but are useful for targeting behavior changes using [`when`](when_node.md) nodes.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `match` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `tag-name` - The name of a tag to make active.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
project "Harvest Engine" {
    when system=windows {
        tags { win32 }
    }

    when tags=win32 {}
}
```
