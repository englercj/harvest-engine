# `dependencies` node

A set of modules, files, or system libraries that a module depends on.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `modify` - Do not modify the set of items. Only update properties of matched items.

## Properties

None.

## Children

- `module-or-library-name` - Required. The name of a module or system library to depend on.
    * Dependencies can optionally specify a `kind` property, detailed below.
    * Dependencies can optionally specify a `whole_archive` property, detailed below.

### `kind` property

The `kind` property is a string value that informs the build system what kind of relationship this module has with the dependency.

The valid kind values are:

- `default` - Create an include, link, and/or build order dependency on the module, depending on the `kind` of the module. This is the default behavior.
- `link` - Create a link and build order dependency, do not make include directories available.
- `order` - Create a build order dependency, do not make include directories available nor link the output.
- `include` - Create an include dependency, do not change the build order nor link the output.
- `file` - Create a link dependency on a library file. The path is relative to the install directory of the plugin.
- `system` - Create a link dependency on a system library. The library must be available in the library search path.
    * HE Make will automatically apply proper naming conventions for the target `system`, so don't specify prefixes (such as `lib`) or suffixes (such as `.a`) to system library dependencies.
    * For XCode frameworks, *do* include the `.framework` extension. For example, `CoreFoundation.framework`.

### `whole_archive` property

The `whole_archive` property is a boolean value that, when set to true, configures the compiler to include every object in an archive when linking. Normally the compiler will search archives to find only the object files necessary for linking.

The valid values are:

- `#false` - Link only object files that are used. This is the default behavior.
- `#true` - Link all object files.

## Scopes

- [`module`](module_node.md)
- [`public`](public_node.md)

## Example

```kdl
dependencies { he_core; pthread kind=system }
```
