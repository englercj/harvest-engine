# `files` node

A set of file paths and how to treat them.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add matched items to the set, or update it if it already exists. This is the default behavior.
    * `remove` - Remove matched the items from the set.
    * `update` - Update properties of matched items if they exist in the set.
        - Can also be used in a [`project`](project_node.md) to specify default actions for files.

## Properties

None.

## Children

- `file-glob` - Required. A glob pattern for the files to include in the module. See [paths](../paths.md) for details.
    * File globs may also specify a string property: `action`, which is detailed below.
    * File globs may also specify a string property: `build_rule`, which is detailed below.
    * File globs may also specify a boolean property: `build_exclude`, which is detailed below.

### `action` property

The `action` property informs the build system how the files should be handled.

The valid action values are:

- `default` - Chose an action based on the file's extension. This is the default behavior.
- `appxmanifest` - Treat the file as an AppxManifest (UWP package manifest).
- `build` - Compile and link as source code files. This is the default action.
- `copy` - Copy the files to the target directory.
- `framework` - Treat the file as an xcode framework.
- `image` - Treat the file as an image.
- `manifest` - Treat the file as a Windows application manifest.
- `natvis` - Natvis files used for debugging visualizations.
- `none` - Do nothing with the files.
- `resource` - Copy or embed the files with project resources.

### `build_rule` property

The `build_rule` property is a string value that defines what build rule should be used for building the files. This property is only relevant when the `action` property is set to `build`.

By default a rule is chosen automatically based on file extension. Custom rules can be defined using the [`build_rule`](build_rule_node.md). The built-in rules are:

- `default` - Select the best rule based on file extension. This is the default behavior.
- `asm` - Compile and link as Assembly source code files.
- `c` - Compile and link as C source code files.
- `cpp` - Compile and link as C++ source code files.
- `csharp` - Compile and link as C# source code files.
- `include` - Treat as include files.
- `objc` - Compile and link as Objective-C source code files.
- `objcpp` - Compile and link as Objective-C++ source code files.
- `midl` - Compile and link as MIDL source code files.

### `build_exclude` property

The `build_exclude` property is a boolean value that informs the build system that this file should be excluded from build operations. This property is only relevant when the `action` property is set to `build`.

This is distinct from the `none` action in that the file is properly configured for building but is excluded for some reason. This is particularly useful if you have certain configurations where files should be excluded from the build, but otherwise should be built normally. An example of this is below.

```kdl
// This configuration will always have the files available, but exclude them from the build
// when their proper platform isn't active.
files {
    "src/source.cpp",
    "src/source.win32.cpp", // only for win32 targets
    "src/source.linux.cpp", // only for linux targets
}

when system=!linux { files modify { "src/source.linux.cpp" build_exclude=#true } }
when system=!windows { files modify { "src/source.win32.cpp" build_exclude=#true } }
```

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
