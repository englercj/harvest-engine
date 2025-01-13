# `public` node

The public node contains all the build configuration for a module that is public to other modules. That is, the child nodes are used to configure this module and any module that depends on this one.

## Arguments

None.

## Properties

None.

## Children

- [`defines`](defines_node.md)
- [`dependencies`](dependencies_node.md)
- [`include_dirs`](include_dirs_node.md)
- [`lib_dirs`](lib_dirs_node.md)

## Scopes

- [`module`](module_node.md)

## Example

```kdl
public {
    dependencies { mimalloc }
    include_dirs { "include" }
    lib_dirs { "lib" }
}
```
