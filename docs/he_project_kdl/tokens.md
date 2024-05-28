# Tokens

Tokens are a way to include dynamic values into strings throughout the HE Make Project files.

The syntax for inserting a token value is: `${context.property:transformer}`.

- `context` is the name of any node that you are a child of, or the name of a Global Context as defined below.
- `property` is the name of a property on that node, or a Computed Property as defined below.
- `transformer` is optional and applies some transformation to the property value before emitting it.

For example:

```kdl
project "Test" {
    "${project.name:lower}" // emits: "test"
}
```

## Global Contexts

Global contexts are contexts that exist outside the project structure. As such, they are always available for use in any scope.

- `cmd` - Provides a way to insert common commands in a portable way. Valid properties are:
    * `chdir` - Emits `chdir {arg}` on windows and `cd {arg}` elsewhere.
    * `copy_file` - Emits `copy /B /Y {args}` on windows and `cp -f {args}` elsewhere.
    * `copy_dir` - Emits `xcopy /Q /E /Y /I {args}` on windows and `cp -rf {args}` elsewhere.
    * `del_file` - Emits `del {arg}` on windows and `rm -rf {arg}` elsewhere.
    * `del_dir` - Emits `rmdir /S /Q {arg}` on windows and `rm -rf {arg}` elsewhere.
    * `make_dir` - Emits `IF NOT EXIST {arg} (mkdir {arg})` on windows and `mkdir -p {arg}` elsewhere.
    * `move` - Emits `move /Y {args}` on windows and `mv -f {args}` elsewhere.
    * `touch` - Emits `type nul >> {arg} && copy /B {arg}+,, {arg}` on windows and `touch {arg}` elsewhere.
- `configuration` - Provides access to properties of the currently active configuration. Valid properties are:
    * `name` - The name of the active configuration.
- `platform` - Provides access to properties of the currently active platform. Valid properties are:
    * `name` - The name of the active platform.
    * `system` - The system for the active platform.
    * `architecture` - The architecture for the active platform.

## Computed Properties

A computed property is one that wasn't explicitly written into the KDL file, but was computed by HE Make and made available to tokens.

Below is a reference of the computed properties for each node. Note that not all nodes provide computed properties.

- `*`
    * `_argN` - A special computed property available to all nodes that provides access to that node's arguments, where `N` is the argument index. For example, `_arg0` is the first argument, `_arg1` is the second argument, etc.
- `project`
    * `name` - The name of the project, equivalent to `_arg0`.
    * `path` - The full path to the file where the project is defined.
- `plugin`
    * `name` - The name of the plugin, equivalent to `_arg0`.
    * `path` - The full path to the file where the plugin is defined.
    * `install_dir` - The directory where the plugin was installed.
- `module`
    * `name` - The name of the module, equivalent to `_arg0`.
    * `path` - The full path to the file where the module is defined.
    * `build_target` - The full path to the build output file.
    * `link_target` - The full path to the linker output file.
    * `gen_dir` - The full path to the directory where generated files should be placed.

## Transformers

Transformers modify the retrieved value before token replacement. Multiple transformers can be chained together to form complex transformations.

- `lower` - Converts the value to lower-case. For example, the `lower` of `Hello` is `hello`.
- `upper` - Converts the value to upper-case. For example, the `upper` of `Hello` is `HELLO`.
- `trim` - Trims whitespace on either side of the value. For example, the `trim` of ` Hello ` is `Hello`.
- `dirname` - Gets the directory name of a path. For example, the `dirname` of `/home/file.cpp` is `/home`.
- `basename` - Gets the base name of a path. For example, the `basename` of `/home/file.cpp` is `file.cpp`.
- `extname` - Gets the extension name of a path. For example, the `extname` of `/home/file.cpp` is `cpp`.
- `extension` - Gets the extension of a path. For example, the `extension` of `/home/file.cpp` is `.cpp`.
- `noextension` - Gets the path without the extension. For example, the `noextension` of `/home/file.cpp` is `/home/file`.
