# `build_event` node

Define a set of commands to run during a build lifecycle event.

## Arguments

1. (string) - Required. The event to run the commands for. Valid values are:
    * `prebuild` - Run before starting build procedures.
    * `build` - Run after the build has completed.
    * `prelink` - Run after sources have been compiled, but before objects are linked. Only valid for C++ projects.
    * `link` - Run after objects have been linked. Only valid for C++ projects.
    * `preclean` - Run before starting build clean procedures.
    * `clean` - Run after the build has been cleaned.

## Properties

- `message` (string) - Optional. The message to display before running the build event commands.

## Children

- [`command`](command_node.md) - Multiple command nodes will be run in sequence.

## Scopes

- [`module`](module_node.md)

## Example

```kdl
build_event prebuild message="Copying dependencies..." {
    command cmd.copy_file some_file some_target_dir
}
```
