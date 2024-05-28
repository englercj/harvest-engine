# `github` node

Defines how to install a plugin from a GitHub repository.

## Arguments

None.

## Properties

- `user` (string) - Required. The name of the user or organization that owns the repository.
- `repo` (string) - Required. The name of the repository to install.
- `ref` (string) - Required. A commit sha, tag name, or branch name to install.

## Children

None.

## Scopes

- [`install`](install_node.md)

## Example

```kdl
install {
    github user=ocornut repo=imgui ref=823a1385a269d923d35b82b2f470f3ae1fa8b5a3
}
```
