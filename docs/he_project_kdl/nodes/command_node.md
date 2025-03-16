# `command` node

Represents a command to be run for a [`build_rule`](build_rule_node.md) or [`build_event`](build_event_node.md).

## Arguments

1. (string) - Required. The command to be run. Built-in commands that can be used for cross-platform support are:
    * `cmd.chdir` - Emits `chdir {args}` on windows and `cd {args}` elsewhere.
    * `cmd.copy_file` - Emits `copy /B /Y {args}` on windows and `cp -f {args}` elsewhere.
    * `cmd.copy_dir` - Emits `xcopy /Q /E /Y /I {args}` on windows and `cp -rf {args}` elsewhere.
    * `cmd.del_file` - Emits `del {args}` on windows and `rm -rf {args}` elsewhere.
    * `cmd.del_dir` - Emits `rmdir /S /Q {args}` on windows and `rm -rf {args}` elsewhere.
    * `cmd.make_dir` - Emits `IF NOT EXIST {args} (mkdir {args})` on windows and `mkdir -p {args}` elsewhere.
    * `cmd.move` - Emits `move /Y {args}` on windows and `mv -f {args}` elsewhere.
    * `cmd.touch` - Emits `type nul >> {args} && copy /B {args}+,, {args}` on windows and `touch {args}` elsewhere.
2. (string) - Optional. Additional arguments for the command.

## Properties

None.

## Children

None.

## Scopes

- [`build_event`](build_event_node.md)
- [`build_rule`](build_rule_node.md)

## Example

```kdl
command echo "the file: ${file.path}"
command cmd.make_dir ${file.path}
```
