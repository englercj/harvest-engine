# `import` node

Imports all the nodes from a KDL file into the scope it is written into.

The behavior of importing a KDL file is equivalent to if you had written the nodes into the parent scope itself.

## Arguments

1. (string) - Required. The path to the KDL file to import. Relative paths are relative to the file where the `import` is written. The file to import is searched for in the following order:
    * The path as a file name
    * The path as a file name with `.kdl` appended, if not already specified.
    * The path as a directory containing an `he_plugin.kdl` file.

## Properties

None.

## Children

None.

## Scopes

- [`install`](install_node.md)
- [`module`](module_node.md)
- [`plugin`](plugin_node.md)
- [`project`](project_node.md)

## Example

```kdl
// Import the file we want directly
import "./he_plugin.kdl"

// Import the same file with an implicit extension
import "./he_plugin"

// Import the `myplugin/he_plugin.kdl` file from a directory
import "./myplugin"
```
