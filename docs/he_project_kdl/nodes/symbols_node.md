# `symbols` node

Configure the toolset's symbol table generation behavior.

## Arguments

1. (string) - Required. The type of symbol table, if any, to generate. Valid values are:
    * `default` - Use the toolset's default behavior. This is the default value.
    * `on` - Enable symbol generation.
    * `off` - Use the Release runtime libraries.

## Properties

- `embed` (boolean) - Optional. When true symbols are embedded into the executable. The default value is `#false`, which will write symbols to a separate file.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
configuration Debug {
    symbols on
}

configuration Release {
    symbols off
}
```
