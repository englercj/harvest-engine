# `platform` node

Defines a build platform for a project, which specifies the target system and architecture to build for.

Platforms have arbitrary names and offer one axis of configuration for your project. The other axis is [`configuration`](configuration_node.md). If no platforms are specified in the project structure, then a default platform is generated based on the host system. This platform will be configured to reasonable defaults to target the host system.

## Arguments

1. (string) - Required. The name of the platform.

## Properties

- `arch` (string) - Required. The architecture to target. Valid values are:
    * `x86` - 32-bit x86.
    * `x86_64` - 64-bit x86.
    * `arm` - 32-bit ARM (aarch32).
    * `arm64` - 64-bit ARM (aarch64).
- `system` (string) - Required. The operating system to target. Valid values are:
    * `linux`
    * `wasm`
    * `windows`
- `toolset` (string) - Optional. The name of the toolset to use for building this platform. By default this will be set to a default based on the `system` property. For valid values see the [`toolset`](toolset_node.md) node.
- `default` (boolean) - Optional. When true, this platform is set as the default selection.
- `version` (string) - Optional. The version of the system to target. Default is `latest`.

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
platform Win64 system=windows arch=x86_64

when platform=Win64 {
    hosts { windows }
    actions { vs2022 }
    defines { HE_PLATFORM_WIN64 }
    tags { windows, win32 }
}
```
