# `system` node

Configures the settings for a target system, such as the SDK version to use.

Note that this node does not set which system is targeted for compilation. That is handled by the [`platform`](platform_node.md) node.

## Arguments

1. (string) - Required. The name of the system to configure. Valid values are:
    * `dotnet`
    * `linux`
    * `wasm`
    * `windows`

## Properties

- `version` (string) - Optional. The version of the system to target. Default is `latest`.
    * For `dotnet` targets this is the [Target Framework Moniker (TFM)](https://learn.microsoft.com/en-us/dotnet/standard/frameworks).
    * For `windows` targets this is the [Windows SDK Version](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/).

## Children

None.

## Scopes

- [`project`](project_node.md)

## Example

```kdl
system windows version=10.0.22621.0
```
