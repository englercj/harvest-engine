# `plugin` node

Defines a plugin that can be imported into a project.

## Arguments

1. (string) - Required. A globally unique identifier for the plugin. This identifier should be unique not only within your project, but globally. To that end, it is recommended you agree on a unique string for your organization that can be prefixed to the id of all plugins you develop.

## Properties

- `version` (string) - Required. An arbitrary version string identifying the version of the plugin. There are currently no defined semantics around the version string or its format.
- `license` (string) - Optional. A SPDX license identifier (https://spdx.org/licenses/), or "UNLICENSED", or "SEE LICENSE IN <filename>". Default is "UNLICENSED".

## Children

- [`authors`](authors_node.md)
- [`build_rule`](build_rule_node.md)
- [`module`](module_node.md)
- [`install`](install_node.md)

## Scopes

- [`project`](project_node.md)

## Example

```kdl
plugin he.core version="0.1.0" license="UNLICENSED" name="Harvest Core" description="Core utilities and platform abstractions." {
    authors { "Chad Engler" email=englercj@live.com }
}
```
