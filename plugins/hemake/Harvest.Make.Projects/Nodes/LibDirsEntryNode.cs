// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LibDirsEntryNode(KdlNode node, INode? scope) : NodeBase<LibDirsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        LibDirsNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "system", NodeValueDef_Bool.Optional(false) },
    };

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
