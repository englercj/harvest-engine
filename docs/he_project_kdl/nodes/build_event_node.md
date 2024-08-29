# `build_event` node

Define a set of commands to run during a build lifecycle event.

## Arguments

1. (string) - Required. The event to run the commands for. Valid values are:
    * `prebuild` - Run before starting build procedures.
    * `postbuild` - Run after the build has completed.
    * `prelink` - Run after source files have been compiled, but before they are linked.

## Properties

- `message` (string) - Optional. The message to display before running the build event commands.

## Children

- `build-command` - The command to run. There can be multiple commands during an event.

## Scopes

- [`module`](module_node.md)

## Example

```kdl
build_event prebuild message="Copying dependencies..." {
    "${cmd.copy_file} some_file some_target_dir"
}
```
