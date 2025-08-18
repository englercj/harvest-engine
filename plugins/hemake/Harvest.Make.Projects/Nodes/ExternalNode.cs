// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ExternalNodeTraits : NodeBaseTraits
{
    public override string Name => "external";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "warnings", NodeValueDef_Enum<EWarningsLevel>.Optional(EWarningsLevel.Default) },
        { "fatal", NodeValueDef_Bool.Optional(false) },
        { "angle_brackets", NodeValueDef_Bool.Optional(true) },
    };
}

public class ExternalNode(KdlNode node) : NodeBase<ExternalNodeTraits>(node)
{
    public EWarningsLevel WarningsLevel => GetEnumValue<EWarningsLevel>("warnings");
    public bool Fatal => GetBoolValue("fatal");
    public bool AngleBrackets => GetBoolValue("angle_brackets");
}
