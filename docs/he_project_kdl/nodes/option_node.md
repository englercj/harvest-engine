# `option` node

Defines a new command-line option that can be set to a value, and checked by

## Arguments

1. (string) - Requires. The name of the option to define. This is used as the name on the command-line and as the name used in the `option` property of a [`when`](when_node.md) node.

## Properties

- `default` (boolean|number|string) - Optional. Overrides the default value for the type with a manually specified one. The value specified here must be of the correct type (e.g. `type=bool default=foo` is an error).
- `env` (string) - Optional. When specified the value will be pulled from the named environment variable if no value is specified on the command-line.
- `help` (string) - Optional. The help text to display for this option on the command-line.
- `type` (string) - Optional. The type of value the option can hold. Valid values are:
    * `bool` - The option is set or unset (`#true` or `#false`), and defaults to `#false`. This is the default type.
    * `int` - The option holds a 64-bit signed integral value, and defaults to `0`.
    * `uint` - The option holds a 64-bit unsigned integral value, and defaults to `0`.
    * `float` - The option holds a 64-bit floating point value, and defaults to `0.0`.
    * `string` - The option holds a string value, and defaults to `""`.

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
// Bool option called `asan` that is `#false` by default
option asan help="Enables Address Sanitizer"

// String option called `myenvvar` which can be specified on the command-line,
// and if not specified will fall back to the environment variable `SOME_HE_ENV_VAR`,
// and if not present there will fall back to the default value of `pizza`.
option myenvvar type=string default=pizza env=HE_SOME_ENV_VAR
```
