# `build_rule` node

Define a custom build rule that can be used for files that use the `build` action in a [`files`](files_node.md) node.

For complex tool integrations that need parameters, companion modules, dependency injection, or other graph mutations, use HE Make's C# extension capabilities instead. `build_rule` is the low-level primitive for custom commands only.

## Arguments

- (string) - Required. A globally unique name for the rule that can be referenced elsewhere.

## Properties

- `message` (string) - Optional. The message to display before running the custom build commands.
- `link_output` (boolean) - Optional. `#false` to disable linking of `.obj` outputs. Default: `#true`.

## Children

- [`command`](command_node.md) - Multiple command nodes will be run in sequence.
- [`inputs`](inputs_node.md)
- [`outputs`](outputs_node.md)

### Token Contexts

In the properties and children of the `build_rule` node an additional [Token Context](../tokens.md) is available called `file`. This context has the following properties:

- `path` - The full path to the file the build rule is operating on.
- `gen_dir` - The full path to the directory where generated files should be placed.
    * This directory is the module's `gen_dir` followed by the path to this file relative to the module.

## Scopes

- [`module`](module_node.md)
- [`plugin`](plugin_node.md)
- [`project`](project_node.md)

## Example

```kdl
build_rule my_schema_compile message="Custom compiling schema: ${file.path}" {
    // Run the schemac executable to compile the input file
    command "${module[he_schemac].build_target} -t c++ -o ${file.gen_dir} ${file.path}"

    // Add the schemac executable as as input so if it changes, this rule gets run again.
    // "${file.path}" is always treated as an input, no need to specify it here
    inputs {
        "${module[he_schemac].build_target}"
    }

    // Specify the output files for dependency tracking
    outputs {
        "${file.gen_dir}/${file.path:basename}.hsc.h"
        "${file.gen_dir}/${file.path:basename}.hsc.cpp"
    }
}
```
