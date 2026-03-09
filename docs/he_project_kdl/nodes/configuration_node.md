# `configuration` node

Defines a build configuration for a project.

Configurations have arbitrary names and offer one axis of configuration for your project. The other axis is [`platform`](platform_node.md). If no configurations are specified in the project structure, then two default configurations are generated: `Debug` and `Release`.

By default a configuration named `Debug` will disable optimizations and a configuration named `Release` will enable optimizations. This can be easily overridden by specifying custom settings as shown in the example below.

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
configuration Release

when configuration.name=Debug {
    defines { _DEBUG; DEBUG }
    optimize off
    runtime debug
    symbols on
}

when configuration.name=Release {
    defines { NDEBUG }
    optimize speed
    runtime release
    symbols on
}
```
