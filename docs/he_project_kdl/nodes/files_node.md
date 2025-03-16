# `files` node

A set of file paths and how to treat them.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.
        - Can also be used in a [`project`](project_node.md) to specify default actions for files.

## Properties

None.

## Children

- `file-glob` - Required. A glob pattern for the files to include in the module. See [paths](../paths.md) for details.
    * File globs may also specify a string property: `action`, which is detailed below.
    * File globs may also specify a string property: `rule`, which is detailed below.

### `action` property

The `action` property informs the build system how the files should be handled.

The valid action values are:

- `default` - Chose an action based on the file's extension. This is the default behavior.
- `appxmanifest` - Treat the file as an AppxManifest (UWP package manifest).
- `build` - Compile and link as source code files. This is the default action.
- `copy` - Copy the files to the target directory.
- `framework` - Treat the file as an xcode framework.
- `image` - Treat the file as an image.
- `natvis` - Natvis files used for debugging visualizations.
- `none` - Do nothing with the files.
- `resource` - Copy or embed the files with project resources.

### `rule` property

The `rule` property is only relevant when the `action` property is set to `build`. It defines what build rule should be used for the files. By default a rule is chosen automatically based on file extension. Custom rules can be defined using the [`build_rule`](build_rule_node.md). The built-in rules are:

- `default` - Select the best rule based on file extension. This is the default behavior.
- `asm` - Compile and link as Assembly source code files.
- `c` - Compile and link as C source code files.
- `cpp` - Compile and link as C++ source code files.
- `csharp` - Compile and link as C# source code files.
- `fx` - Compile and link as HLSL source code files.
- `include` - Treat as include files.
- `objc` - Compile and link as Objective-C source code files.
- `objcpp` - Compile and link as Objective-C++ source code files.
- `midl` - Compile and link as MIDL source code files.
- `swift` - Compile and link as Swift source code files.

## Scopes

- [`module`](module_node.md)

## Example

```kdl
files {
    // Add files using the default 'build' action and a rule chosen based on file extension.
    "include/**"
    "src/**"

    // Add image files that get copied to the output directory.
    "images/**" action=copy

    // Add schema files using the 'build' action and a custom rule 'he_schema_compile' that
    // is defined in a C# extension and has custom configuration.
    "schema/**.hsc" rule=he_schema_compile {
        dependencies { "he_editor" }
        include_dirs { "assets/include" }
        targets { "c++" }
    }
}

// Remove our pizza file that was added in the block above.
files remove {
    "src/one_file.pizza"
}

// Change the properties of a file that was added in the block above.
// Without the `modify` argument we may inadvertently add files we don't want.
files match {
    "**.pizza" action=none
}
```
