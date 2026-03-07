// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

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

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        resolvedNode = resolver.CreateResolvedNode(source, includeChildren: false);

        if (!source.HasValue("system")
            && target.TryGetValue("system", out bool isSystem))
        {
            resolvedNode.Properties["system"] = new KdlBool(isSystem) { SourceInfo = source.SourceInfo };
        }

        return true;
    }

    public override INode CreateNode(KdlNode node) => new LibDirsEntryNode(node);
}

public class LibDirsEntryNode(KdlNode node) : NodeSetEntryBase<LibDirsEntryNodeTraits, LibDirsNode>(node)
{
    public string Path => ResolveSinglePath(Node.Name);

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
