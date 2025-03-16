// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects.Nodes;

public class CommandNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        BuildEventNode.NodeName,
        BuildRuleNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlString.Required(),
        NodeKdlString.Optional(),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? CommandName => TryGetStringValue(0);
    public string? CommandArgs => TryGetStringValue(1);

    public string? GetCommandString()
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

    protected override NodeValidationResult ValidateArguments()
    {
        if (Node.Arguments.Count == 0)
        {
            return NodeValidationResult.Error($"'{Name}' node must have at least one argument.");
        }

        for (int i = 0; i < Node.Arguments.Count; ++i)
        {
            if (Node.Arguments[i] is not KdlString)
            {
                return NodeValidationResult.Error($"Arguments of '{Name}' nodes must be strings.");
            }
        }

        return NodeValidationResult.Valid;
    }
}
