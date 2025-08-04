// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects.Nodes;

public class CommandNode(KdlNode node, INode? scope) : NodeBase<CommandNode>(node, scope)
{
    public static string NodeName => "command";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        BuildEventNode.NodeName,
        BuildRuleNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
        NodeValueDef_String.Optional(),
    ];

    public string CommandName => GetStringValue(0);
    public string? CommandArgs => TryGetStringValue(1);

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

    protected override void ValidateArguments()
    {
        // Intentionally not calling base.ValidateArguments() because we have custom validation.
        //base.ValidateArguments();

        if (Node.Arguments.Count == 0)
        {
            throw new NodeValidationException(this, $"'{NodeName}' nodes must have at least one string argument.");
        }

        for (int i = 0; i < Node.Arguments.Count; ++i)
        {
            if (Node.Arguments[i] is not KdlString)
            {
                throw new NodeValidationException(this, $"Arguments of '{Name}' nodes must be strings.");
            }
        }
    }
}
