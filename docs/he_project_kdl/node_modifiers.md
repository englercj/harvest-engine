# Node Modifiers

Node modifiers are a syntax to modify the behavior of a node.

## Generators

Generates are nodes that are evaluated at parsing time to generate nodes in their place. Generator nodes are prefixed with a colon (`:`). Generator nodes are replaced with the nodes they generate in the parsed project structure.

The built-in generators are:

- [`:foreach`](#foreach-generator) - generates child nodes once for each matching node

> [!NOTE]
> Nested generators, or extensions nested within a generator, are not currently supported.

### Foreach Generator

The `:foreach` generator finds nodes matching the given filter and generates nodes for each match using the child nodes as a template. Child nodes can use the special token context `_entry` to represent the matched entry of the current iteration. It will behave as if it was a token context of the type being searched for. For example, when searching for modules the `_entry` context will act like the `module` token context. See [tokens](tokens.md) for more info.

#### Arguments

- (string) - Required. The node type to search for. Valid values are:
    * `plugin` - Search for matching plugins.
    * `module` - Search for matching modules.

#### Properties

Each property checks if a property of the same name on the context matches the value. For example, `arch=x86_64` checks that the `arch` property of the context is equal to `x86_64`. You can check arguments by using a special `_argN` property, where `N` is the index of the argument to check. For example, `_arg0="foo"` will check that the first argument is equal to `"foo"`.

Values can be logically combined with `||` (or), `&&` (and), or `^` (xor). Conditions are evaluated from left-to-right, and parenthesis (`()`) may be used to group conditions. The equality check can be negated by prefixing the value with `!`.


#### Children

Child nodes of the `:foreach` generator are treated as a template to generate nodes. Each child node is evaluated at parse time and generates a node in the project structure, for each matched item. Any node that is valid in the parent scope, is valid in the generator.

Generators cannot currently be nested.

#### Example

```kdl
// This is a console app that links all test projects and runs them.
module he_test_runner kind=console_app group="engine/tests" {
    files { "test_runner/main.cpp" }

    public {
        dependencies {
            // Normal dependency on `he_core`.
            he_core

            // Generate a dependency node for each module in the "engine/tests" group that
            // has `kind=lib_static`.
            :foreach module group="engine/tests" kind=lib_static {
                "${_entry.name}" whole_archive=#true
            }
        }
    }
}
```

## Extensions

Extensions allow you to modify the definitions of nodes defined elsewhere in the project. This can be useful when a module is defined by a plugin outside of your source control and you want to modify it's behavior. Properties and children of the extension node are merged into the existing definition. If no existing definition is found, the extension is silently ignored.

For example, you may want to inject your own library into the Harvest Editor application. An easy way to do this is through extensions.

```kdl
plugin mygame version="1.0.0" license="UNLICENSED" {
    // Define a module that has some cool editor functionality for mygame.
    module mygame_editor kind=lib_static group="mygame" {
        files { "cool_editor_stuff.cpp" }
    }

    // Extend the harvest editor's module definition to include a dependency
    // on mygame's editor module so it is linked and loaded.
    +module he_editor {
        dependencies { mygame_editor }
    }
}
```

This extension is only applied if the `he_editor` module is defined elsewhere in the project structure. If not, it will silently do nothing. If you want to receive errors when an extension cannot be resolved, you can mark it as required:

```
+module he_editor required=#true {
    // ...
}
```

This will emit an error if no `he_editor` module can be found to extend.

### Valid Nodes

These are the nodes that can be used as generators:

- [`module`](nodes/module_node.md)
- [`plugin`](nodes/plugin_node.md)
