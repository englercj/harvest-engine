# `when` node

Wraps one or more other nodes that are applied in the parent scope only if the conditions are met.

## Arguments

1. (string) - Optional. How multiple properties should be treated. Valid values are:
    * `all` - All properties must be true. This is the default behavior.
    * `any` - At least one property must be true.
    * `one` - Exactly one property must be true.

## Properties

Values can be logically combined with `||` (or), `&&` (and), or `^` (xor). Conditions are evaluated from left-to-right, and parenthesis (`()`) may be used to group conditions. The equality check can be negated by prefixing the value with `!`.

There are two kinds of `when` condition keys:

- Special keys:
  - `option` (string) - Checks if an option is active, or holds a specific value.
    * `when option=a {}` checks if the `a` option is set with any value.
    * `when option="a:test" {}` checks if the `a` option is set to `test`.
  - `tags` (string) - Checks if the active [`tags`](tags_node.md) include the specified one.
- Token-style keys:
  - Any other key must be a token path and is resolved the same way as a `${...}` token before the expression is evaluated.
  - Common examples are `configuration.name`, `platform.name`, `platform.arch`, `platform.system`, `platform.toolset`, `module.kind`, `module.language`, `plugin.version`, and `host.name`.
  - Unknown bare keys such as `when system=windows {}` are invalid. Use the equivalent token-style key instead, for example `when platform.system=windows {}`.

## Common Replacements

Use these token-style keys instead of the old scalar shorthand forms:

| Old form | New form |
| --- | --- |
| `when configuration=Debug {}` | `when configuration.name=Debug {}` |
| `when platform=Win64 {}` | `when platform.name=Win64 {}` |
| `when system=windows {}` | `when platform.system=windows {}` |
| `when arch=x86_64 {}` | `when platform.arch=x86_64 {}` |
| `when toolset=msvc {}` | `when platform.toolset=msvc {}` |
| `when host=windows {}` | `when host.name=windows {}` |
| `when language=cpp {}` | `when module.language=cpp {}` |
| `when kind=lib_static {}` | `when module.kind=lib_static {}` |

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
when platform.system=windows {}

// system is NOT 'windows'
when platform.system=!windows {}

// arch is 'x86' OR 'x86_64'
when platform.arch="x86 || x86_64" {}

// configuration is 'Debug'
when configuration.name=Debug {}

// active module kind is lib_static
when module.kind=lib_static {}

// option 'a' is specified with any value
when option=a {}

// option 'a' is specified with the value 'test'
when option="a:test" {}
```
