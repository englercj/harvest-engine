# `warnings` node

Sets the level of warnings emitted by the toolset.

## Arguments

1. (string) - Required. How warnings are considered. Valid values are:
    * `default` - Use the toolset's default warning behavior. This is the behavior.
    * `all` - Enable all available warnings for the toolset.
    * `extra` - Enables extra warnings that are not typically enabled by the toolset.
    * `on` - Enables significant warnings from the toolset.
    * `off` - Disable all warnings from the toolset.

## Properties

- `fatal` (boolean) - Optional. Whether to treat warnings as errors.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
warnings extra fatal=#true
```
