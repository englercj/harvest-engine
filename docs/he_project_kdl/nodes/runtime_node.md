# `runtime` node

Set the type of runtime library to use.

## Arguments

1. (string) - Required. The runtime to use. Valid values are:
    * `default` - Automatically choose the correct runtime based on the [`optimize`](optimize_node.md) node. This the default value.
        - When `optimize` is `off` or `debug`, then `debug` libraries are used. Otherwise, `release` libraries are used.
    * `debug` - Use the Debug runtime libraries.
    * `release` - Use the Release runtime libraries.

## Properties

None.

## Children

None.

## Scopes

- [`module`](module_node.md)
- [`project`](project_node.md)

## Example

```kdl
configuration Debug {
    runtime debug
}

configuration Release {
    runtime release
}
```
