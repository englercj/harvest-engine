# `external` node

Sets the compiler behavior for external files.

External files are files that are added to the include search path with an [`include_dirs`](include_dirs_node.md) node with the `external=#true` property set, or any include using angle brackets (`<>`) if the `angle_brackets` property is set to `#true`.

## Arguments

None.

## Properties

- `warnings` (string) - Optional. How warnings are considered. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `all` - Enable all available warnings for the toolset.
    * `extra` - Enables extra warnings that are not typically enabled by the toolset.
    * `on` - Enables significant warnings from the toolset.
    * `off` - Disable all warnings from the toolset.
- `fatal` (boolean) - Optional. Whether to treat warnings as errors.
- `angle_brackets` (boolean) - Optional. When set to true all includes that use angle brackets (`<>`) are considered external. The default is `#false`.

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
external warnings=off fatal=#true angle_brackets=#true
```
