// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsEntryNode(KdlNode node, INode? scope) : NodeBase<IncludeDirsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        IncludeDirsNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "external", NodeValueDef_Bool.Optional(false) },
    };

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
