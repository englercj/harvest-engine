// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ExternalNode(KdlNode node, INode? scope) : NodeBase<ExternalNode>(node, scope)
{
    public static string NodeName => "external";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "warnings", NodeValueDef_Enum<EWarningsLevel>.Optional(EWarningsLevel.Default) },
        { "fatal", NodeValueDef_Bool.Optional(false) },
        { "angle_brackets", NodeValueDef_Bool.Optional(true) },
    };

    public EWarningsLevel WarningsLevel => GetEnumValue<EWarningsLevel>("warnings");
    public bool Fatal => GetBoolValue("fatal");
    public bool AngleBrackets => GetBoolValue("angle_brackets");
}
