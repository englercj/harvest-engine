# `tags` node

A set of tags to activate in the scope. Tags don't modify any behavior by themselves, but are useful for targeting behavior changes using [`when`](when_node.md) nodes.

## Arguments

1. (string) - Required. The list of tags to activate.

## Properties

None.

## Children

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
project "Harvest Engine" {
    when system=windows {
        tags win32
    }

    when tags=win32 {}
}
```
