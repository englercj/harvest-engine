// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public enum ELibDirIsSystem
{
    Inherit,
    True,
    False,
}

public class LibDirsEntryNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        LibDirsNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "system", NodeKdlBool.Optional(false) },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string Path => Node.Name;

    public ELibDirIsSystem IsSystem
    {
        get
        {
            bool? isSystemProp = TryGetBoolValue("System");
            if (isSystemProp is null)
            {
                return ELibDirIsSystem.Inherit;
            }
            return isSystemProp.Value ? ELibDirIsSystem.True : ELibDirIsSystem.False;
        }
    }
}
