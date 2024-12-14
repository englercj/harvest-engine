# `tags` node

A set of tags to activate in the scope. Tags don't modify any behavior by themselves, but are useful for targeting behavior changes using [`when`](when_node.md) nodes.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `tag-name` - Required. The name of the tag to add, remove, or modify.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
project "Harvest Engine" {
    when system=windows arch=x86 {
        tags { win32 }
    }

    when tags=win32 {
        // windows x86 config
    }

    // you can also activate tags based on other tags
    when tags=win32 {
        tags { definitelyWin32 }
    }
}
```
