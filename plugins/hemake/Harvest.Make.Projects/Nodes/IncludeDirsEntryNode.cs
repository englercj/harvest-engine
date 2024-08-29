// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsEntryNode(KdlNode node) : NodeBase(node)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        IncludeDirsNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "external", NodeKdlValue<KdlBool>.Optional },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? Path => Node.Name;
    public bool IsExternal => GetBoolValue("external") ?? false;
    public bool ExternalOverride => GetBoolValue("external") is not null;
}
