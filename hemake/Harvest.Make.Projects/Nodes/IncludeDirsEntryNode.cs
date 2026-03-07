// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsEntryNodeTraits : NodeSetEntryBaseTraits<IncludeDirsNode>
{
    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "external", NodeValueDef_Bool.Optional(false) },
    };

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        resolvedNode = resolver.CreateResolvedNode(source, includeChildren: false);

        if (!source.HasValue("external")
            && target.TryGetValue("external", out bool isExternal))
        {
            resolvedNode.Properties["external"] = new KdlBool(isExternal) { SourceInfo = source.SourceInfo };
        }

        return true;
    }

    public override INode CreateNode(KdlNode node) => new IncludeDirsEntryNode(node);
}

public class IncludeDirsEntryNode(KdlNode node) : NodeSetEntryBase<IncludeDirsEntryNodeTraits, IncludeDirsNode>(node)
{
    public string Path => ResolveSinglePath(Node.Name);

    public bool IsExternal
    {
        get
        {
            if (TryGetValue("external", out bool isExternal))
            {
                return isExternal;
            }

            if (Node.Parent is not null && Node.Parent.Name == IncludeDirsNode.NodeTraits.Name)
            {
                if (Node.Parent.TryGetValue("external", out bool isParentExternal))
                {
                    return isParentExternal;
                }
            }

            return false;
        }
    }
}
