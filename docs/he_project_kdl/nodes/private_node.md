# `private` node

The private node contains all the build configuration for a module that is private to that module. That is, the child nodes are only used to configure this module and aren't exposed to other modules that depend on this one.

## Arguments

None.

## Properties

None.

## Children

- [`defines`](defines_node.md)
- [`dependencies`](dependencies_node.md)
- [`include_dirs`](include_dirs_node.md)

## Scopes

- [`module`](module_node.md)

## Example

```kdl
private {
    dependson { he_core }
    include_dirs { "src" }
}
```
