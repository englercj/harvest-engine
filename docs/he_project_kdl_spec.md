# Project File Spec

Projects are specified by a special file, `he_project.kdl`, using the [KDL v2.0.0][kdl2] syntax. This file contains the definition of a Harvest Project structure. A Harvest Project provides definitions for one or more Plugins, which in turn provide definitions for Modules.

## Plugins

Plugins act as the unit of installation and sharing. They are a logical grouping of Modules and provide for proper fetching and setup of those Modules. It can also provide extensions to HE Make, or extensions to module that are defined by other plugins.

See the [`project`](he_project_kdl/nodes/project_node.md) node for a formal definition of the node.

## Modules

A Module is the unit of compilation. It defines some number of input files that are used to generate an output, such as an executable or library. Modules can express dependency relationships between each other.

See the [`module`](he_project_kdl/nodes/module_node.md) node for a format definition of the node.

## Syntax

Aside from the structural syntax as defined by [KDL v2.0.0][kdl2] there are a few special behaviors in the project structure syntax. Be sure to read about [filesystem paths](he_project_kdl/paths.md), [string tokens](he_project_kdl/tokens.md), and [node modifiers](he_project_kdl/node_modifiers.md).

<!-- link index -->
[kdl2]: https://github.com/kdl-org/kdl/blob/kdl-v2/SPEC.md
