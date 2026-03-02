// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class LibDirsEntryNodeTraits : NodeSetEntryBaseTraits<LibDirsNode>
{
    public override IReadOnlyList<string> ValidScopes =>
    [
        LibDirsNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "system", NodeValueDef_Bool.Optional(false) },
    };

    public override INode CreateNode(KdlNode node) => new LibDirsEntryNode(node);
}

internal class LibDirsEntryNode(KdlNode node) : NodeSetEntryBase<LibDirsEntryNodeTraits, LibDirsNode>(node)
{
    public string Path => Node.Name;

    public bool IsSystem
    {
        get
        {
            if (TryGetValue("system", out bool isSystem))
            {
                return isSystem;
            }

            if (Node.Parent is not null && Node.Parent.Name == LibDirsNode.NodeTraits.Name)
            {
                if (Node.Parent.TryGetValue("system", out bool isParentSystem))
                {
                    return isParentSystem;
                }
            }

            return false;
        }
    }
}
