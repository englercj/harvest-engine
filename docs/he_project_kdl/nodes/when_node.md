# `when` node

Wraps one or more other nodes that are applied in the parent scope only if the conditions are met.

## Arguments

1. (string) - Optional. How multiple properties should be treated. Valid values are:
    * `all` - All properties must be true. This is the default behavior.
    * `any` - At least one property must be true.
    * `one` - Exactly one property must be true.

## Properties

Each property checks if the context matches the value. For example, `arch=x86_64` checks that active platform's architecture is equal to `x86_64`.

Values can be logically combined with `||` (or), `&&` (and), or `^` (xor). Conditions are evaluated from left-to-right, and parenthesis (`()`) may be used to group conditions. The equality check can be negated by prefixing the value with `!`.

The values you can specify as properties to be checked are:

- `arch` (string) - Optional. Checks if the active platform's architecture matches.
- `configuration` (string) - Optional. Checks if the active configuration name matches.
- `host` (string) - Optional. Checks if the host operating system matches.
- `language` (string) - Optional. Checks if the target compilation is for a particular language (`cpp`, `csharp`, etc).
- `option` (string) - Optional. Checks if an option is active, or holds a specific value.
    * For example, `when option=a {}` checks if the `a` option is set with any value.
    * Another example, `when option="a:test" {}` checks if the `a` option is set to `test`.
- `platform` (string) - Optional. Checks if the active platform name matches.
- `system` (string) - Optional. Checks if the active platform's system matches.
- `tags` (string) - Optional. Checks if the active [`tags`](tags_node.md) include the specified one.
- `toolset` (string) - Optional. Checks if the toolset being used matches.

## Children

- Any node that is valid in the parent scope

## Scopes

- [`install`](install_node.md)
- [`module`](module_node.md)
- [`plugin`](plugin_node.md)
- [`project`](project_node.md)

## Example

```kdl
// system is 'windows'
when system=windows {}

// system is NOT 'windows'
when system=!windows {}

// arch is 'x86' OR 'x86_64'
when arch="x86 || x86_64" {}

// option 'a' is specified with any value
when option=a {}

// option 'a' is specified with the value 'test'
when option="a:test" {}

// option 'a' is specified with the value 'test' OR option 'b' is specified with the value 'test'
when option="a:test || b:test" {}
```
