# `warnings` node

Sets the level of warnings emitted by the toolset.

## Arguments

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

- `level` (string) - Optional. Sets the toolset's warning level. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `all` - Enable all available warnings for the toolset.
    * `extra` - Enables extra warnings that are not typically enabled by the toolset.
    * `on` - Enables significant warnings from the toolset.
    * `off` - Disable all warnings from the toolset.
- `fatal` (boolean) - Optional. Whether to treat all warnings as errors.

## Children

- `warning-name` - Optional. The name of a specific warning to configure further.
    * Warnings can optionally specify a single argument, detailed below.
    * Warnings can optionally specify a `fatal` property, detailed below.

### argument

The argument that a warning may specify is whether or not for it to be enabled or disabled.

The valid values are:

- `enable` - The warning is enabled and will be emitted. This is the default behavior.
- `disable` - The warning is disabled and will not be emitted. The `fatal` property is ignored when the warning is disabled.

### `fatal` property

The `fatal` property is a boolean value that, when set to true, configures the compiler to treat this warning as an error.

The valid values are:

- `#false` - The warning is not treated as an error. This is the default behavior.
- `#true` - The warning is treated as an error.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
warnings level=extra fatal=#true

when toolset=msvc {
    warnings {
        "44668" enable             // A symbol that was not defined was used with a preprocessor directive.
        "44062" enable             // An enumerator has no associated case handler in a switch statement, and there's no default label that can catch it.
    }
}
when toolset="clang || gcc" {
    warnings {
        "undef" enable              // A symbol that was not defined was used with a preprocessor directive.
        "switch" enable             // An enumerator has no associated case handler in a switch statement, and there's no default label that can catch it.
        "old-style-cast" disable
    }
}
```
