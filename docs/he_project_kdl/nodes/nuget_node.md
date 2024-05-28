# `nuget` node

Defines how to install a plugin via NuGet.

## Arguments

1. (string) - Required. The name of the package to install.

## Properties

- `version` (string) - Required. The version of the package to be installed.

## Children

None.

## Scopes

- [`install`](install_node.md)

## Example

```kdl
install {
    nuget Microsoft.Direct3D.DirectStorage version="1.2.1"
}
```
