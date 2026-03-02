// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum ECDialect
{
    [KdlName("default")] Default,
    [KdlName("c11")] C11,
    [KdlName("c17")] C17,
    [KdlName("c23")] C23,
}

public enum ECppDialect
{
    [KdlName("default")] Default,
    [KdlName("cpp14")] Cpp14,
    [KdlName("cpp17")] Cpp17,
    [KdlName("cpp20")] Cpp20,
    [KdlName("cpp23")] Cpp23,
}

public enum ECSharpDialect
{
    [KdlName("default")] Default,
    [KdlName("cs12")] CSharp12,
    [KdlName("cs13")] CSharp13,
}

public class DialectNodeTraits : NodeBaseTraits
{
    public override string Name => "dialect";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
        ModuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "c", NodeValueDef_Enum<ECDialect>.Optional(ECDialect.Default) },
        { "cpp", NodeValueDef_Enum<ECppDialect>.Optional(ECppDialect.Default) },
        { "csharp", NodeValueDef_Enum<ECSharpDialect>.Optional(ECSharpDialect.Default) },
    };

    public override INode CreateNode(KdlNode node) => new DialectNode(node);
}

public class DialectNode(KdlNode node) : NodeBase<DialectNodeTraits>(node)
{
    public ECDialect CDialect => GetEnumValue<ECDialect>("c");
    public ECppDialect CppDialect => GetEnumValue<ECppDialect>("cpp");
    public ECSharpDialect CSharpDialect => GetEnumValue<ECSharpDialect>("csharp");
}
