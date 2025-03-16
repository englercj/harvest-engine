# `build_event` node

Define a set of commands to run during a build lifecycle event.

## Arguments

1. (string) - Required. The event to run the commands for. Valid values are:
    * `prebuild` - Run before starting build procedures.
    * `build` - Run as part of the build procedures.
    * `postbuild` - Run after the build has completed.
    * `prelink` - Run after sources have been compiled, but before objects are linked. Only valid for C++ projects.
    * `postlink` - Run after objects are linked. Only valid for C++ Makefile projects.
    * `clean` - Run as part of build clean procedures. Only valid for Makefile projects.

## Properties

- `message` (string) - Optional. The message to display before running the build event commands.

## Children

- [`command`](command_n ode.md) - Multiple command nodes will be run in sequence.
- [`inputs`](inputs_node.md)
- [`outputs`](outputs_node.md)

## Scopes

- [`module`](module_node.md)

## Example

```kdl
build_event prebuild message="Copying dependencies..." {
    command cmd.copy_file some_file some_target_dir
}
```
