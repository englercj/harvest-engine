# `when` node

Wraps one or more other nodes that are applied in the parent scope only if all the conditions are met.

## Arguments

1. (string) - Optional. How multiple properties should be treated. Valid values are:
    * `and` - All properties must be true. This is the default behavior.
    * `or` - At least one property must be true.
    * `xor` - Exactly one property must be true.

## Properties

- `arch` (string) - Optional. Checks if the active platform's architecture matches.
- `configuration` (string) - Optional. Checks if the active configuration name matches.
- `host` (string) - Optional. Checks if the host operating system matches.
- `language` (string) - Optional. Checks if the target compilation is for a particular language (`cpp`, `csharp`, etc).
- `option` (string) - Optional. Checks if an option is active, or holds a specific value.
    * For example, `when option=asan {}` checks if the `asan` option is set with any value.
    * Another example, `when option="asan:test" {}` checks if the `asan` option is set to `test`.
- `platform` (string) - Optional. Checks if the active platform name matches.
- `system` (string) - Optional. Checks if the active platform's system matches.
- `tags` (string) - Optional. Checks if the active [`tags`](tags_node.md) include the specified one.
- `toolset` (string) - Optional. Checks if the toolset being used matches.

## Children

- Any node that is valid in the parent scope, except for another `when` node.

## Scopes

- [`install`](install_node.md)
- [`module`](module_node.md)
- [`plugin`](plugin_node.md)
- [`project`](project_node.md)

## Example

```kdl
when system=windows {
    import "./windows_only/he_plugin.kdl"
}
```
