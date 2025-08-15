// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LibDirsEntryNodeTraits : NodeSetEntryBaseTraits<LibDirsNode>
{
    public override IReadOnlyList<string> ValidScopes =>
    [
        LibDirsNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "system", NodeValueDef_Bool.Optional(false) },
    };
}

public class LibDirsEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<LibDirsEntryNodeTraits, LibDirsNode>(node, scope)
{
    public string Path => ResolvePath(Node.Name);

    public bool IsSystem
    {
        get
        {
            bool? isSystemProp = TryGetBoolValue("system");
            if (isSystemProp is not null)
            {
                return isSystemProp.Value;
            }

            if (Scope is LibDirsNode libDirsNode)
            {
                return libDirsNode.IsSystem;
            }

            return false;
        }
    }
}
