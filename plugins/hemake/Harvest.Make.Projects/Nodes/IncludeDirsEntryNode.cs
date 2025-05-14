// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsEntryNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        IncludeDirsNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "external", NodeKdlBool.Optional(false) },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string Path => ResolvePath(Node.Name);

    public bool IsExternal
    {
        get
        {
            bool? isExternalProp = TryGetBoolValue("external");
            if (isExternalProp is not null)
            {
                return isExternalProp.Value;
            }

            if (Scope is IncludeDirsNode includeDirsNode)
            {
                return includeDirsNode.IsExternal;
            }

            return false;
        }
    }
}
