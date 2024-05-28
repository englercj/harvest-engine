# `configuration` node

Defines a build configuration for a project.

Configurations have arbitrary names and offer one axis of configuration for your project. The other axis is [`platform`](platform_node.md). If no configurations are specified in the project structure, then two default configurations are generated: `Debug` and `Release`. These will be configured to reasonable default settings to support debugging in `Debug` and a fast runtime in `Release`.

## Arguments

1. (string) - Required. The name of the configuration.

## Properties

None.

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
configuration Debug

when configuration=Debug {
    defines { _DEBUG; DEBUG; HE_CFG_DEBUG }
    runtime debug
    symbols full
}
```
