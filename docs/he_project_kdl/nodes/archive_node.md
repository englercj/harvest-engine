# `archive` node

Defines how to install a plugin via an archive URL.

## Arguments

1. (string) - Required. The URL of the archive to install.

## Properties

- `base` (string) - Optional. The base path to append to the extracted directory where installed files are found.

## Children

None.

## Scopes

- [`install`](install_node.md)

## Example

```kdl
install {
    archive "https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip" base="sqlite-amalgamation-3420000"
}
```
