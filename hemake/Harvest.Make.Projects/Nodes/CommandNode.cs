// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects.Nodes;

public class CommandNodeTraits : NodeBaseTraits
{
    public override string Name => "command";

    public override IReadOnlyList<string> ValidScopes =>
    [
        BuildEventNode.NodeTraits.Name,
        BuildRuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
        NodeValueDef_String.Optional(),
    ];

    protected override void ValidateArguments(KdlNode node)
    {
        // Intentionally not calling base.ValidateArguments() because we have custom validation.
        //base.ValidateArguments();

        if (node.Arguments.Count == 0)
        {
            throw new NodeParseException(node, $"'{node.Name}' nodes must have at least one string argument.");
        }

        for (int i = 0; i < node.Arguments.Count; ++i)
        {
            if (node.Arguments[i] is not KdlString)
            {
                throw new NodeParseException(node, $"Arguments of '{node.Name}' nodes must be strings.");
            }
        }
    }

    public override INode CreateNode(KdlNode node) => new CommandNode(node);
}

public class CommandNode(KdlNode node) : NodeBase<CommandNodeTraits>(node)
{
    public string CommandName => GetValue<string>(0);
    public string? CommandArgs => TryGetValue(1, out string? value) ? value : null;

    public string GetCommandString()
    {
        string? args = CommandArgs;

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return CommandName switch
            {
                "cmd.chdir" => $"chdir {args}",
                "cmd.copy_file" => $"copy /B /Y {args}",
                "cmd.copy_dir" => $"xcopy /Q /E /Y /I {args}",
                "cmd.del_file" => $"del {args}",
                "cmd.del_dir" => $"rmdir /S /Q {args}",
                "cmd.make_dir" => $"IF NOT EXIST {args} (mkdir {args})",
                "cmd.move" => $"move /Y {args}",
                "cmd.touch" => $"type nul >> {args} && copy /B {args}+,, {args}",
                _ => string.IsNullOrEmpty(args) ? CommandName : $"{CommandName} {args}",
            };
        }
        else
        {
            return CommandName switch
            {
                "cmd.chdir" => $"cd {args}",
                "cmd.copy_file" => $"cp -f {args}",
                "cmd.copy_dir" => $"cp -rf {args}",
                "cmd.del_file" => $"rm -rf {args}",
                "cmd.del_dir" => $"rm -rf {args}",
                "cmd.make_dir" => $"mkdir -p {args}",
                "cmd.move" => $"mv -f {args}",
                "cmd.touch" => $"touch {args}",
                _ => string.IsNullOrEmpty(args) ? CommandName : $"{CommandName} {args}",
            };
        }
    }
}
