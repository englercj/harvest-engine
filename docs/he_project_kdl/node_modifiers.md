# Node Modifiers

Node modifiers are a syntax to modify the behavior of a node.

## Generators

Prefixing a node name with a colon (`:`) makes it a generator. Instead of the normal behavior for that node, a generator will instead perform a project-wide search for nodes of that type that meet the filter criteria specified as arguments and properties. The children of generators are nodes that will be parsed as if they were in the parent scope, once for each node matching the generators filter, using the context of the found node.

For example, if you need to access the context of a particular module you can do this:

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
            :module group="engine/tests" kind=lib_static { "${module.name}" whole_archive=#true }
        }
    }
}
```

### Filters

Generators can filter against any argument or property value. Globs are allowed for partial matching in argument values and property values.

For arguments, specify the filter in the same argument index as the node you're searching. For example, module names are specified as the first argument so filtering based on name looks like: `:module he_core`.

For properties, specify the filter using the same property name as the node you're searching. For example, modules specify a `kind` property so filtering based on `kind` looks like: `:module kind=*_app`.

### Valid Nodes

These are the nodes that can be used as generators:

- [`module`](nodes/module_node.md)
- [`plugin`](nodes/plugin_node.md)

## Extensions

While most nodes can be specified multiple times and are additive, there are a few nodes that can only be specified once because they define a unique identifier. The extension syntax allows you to specify a unique node a second time elsewhere in the project structure, and combine the properties and children nodes with other definitions.

For example, you may want to inject your own library into the Harvest Editor application. An easy way to do this is through extensions.

```kdl
plugin mygame version="1.0.0" license="UNLICENSED" {
    // Define a module that has some cool editor functionality for mygame.
    module mygame_editor kind=lib_static group="mygame" {
        files { "cool_editor_stuff.cpp" }
    }

    // Extend the harvest editor's module definition to include a dependency
    // on mygame's editor module.
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


### Why a special syntax?

It may seem odd that a different syntax is used for plugin and module extension, when just making the nodes have additive behavior like other nodes would also solve the issue.

This is true. However, doing so would mean that extensions would end up defining a new module if the original module definition doesn't exist. Consider an example.

Plugin `A` defines module `m`:

```kdl
// A/he_plugin.kdl
plugin A version="1.0.0" license="MIT" {
    module m kind=lib_static {}
}
```

And plugin `B` extends module `m` with an extra dependency:

```kdl
// B/he_plugin.kdl
plugin B version="1.0.0" license="MIT" {
    +module m {
        public {
            dependency { something }
        }
    }
}
```

Now consider a project that includes plugin `B` but not plugin `A`. If there was no special syntax to tell HE Make that the module node in plugin `B` is an extension it would throw an error that `kind` is missing. Or if `kind` was specified, it would define a module `m`. Our desired behavior here is that the `+module m` definition in plugin `B` is inert unless plugin `A` is also included.

Another common case is that `m`, as defined in plugin `A`, could be guarded by a `when` condition. Without a special syntax to know that plugin `B` is intending to extend the module, plugin `B` would end up defining the module in invalid configurations.

### Valid Nodes

These are the nodes that can be extended:

- [`module`](nodes/module_node.md)
- [`plugin`](nodes/plugin_node.md)
