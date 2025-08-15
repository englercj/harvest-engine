// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsEntryNodeTraits : NodeSetEntryBaseTraits<IncludeDirsNode>
{
    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "external", NodeValueDef_Bool.Optional(false) },
    };
}

public class IncludeDirsEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<IncludeDirsEntryNodeTraits, IncludeDirsNode>(node, scope)
{
    public string Path => ResolvePath(Node.Name);

    public bool IsExternal
    {
        get
        {
            if (TryGetBoolValue("external") is bool isExternalProp)
            {
                return isExternalProp;
            }

            if (Scope is IncludeDirsNode includeDirsNode)
            {
                return includeDirsNode.IsExternal;
            }

            return false;
        }
    }
}
