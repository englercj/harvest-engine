// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum ECDialect
{
    [KdlName("default")] Default,
    [KdlName("c99")] C99,
    [KdlName("c11")] C11,
    [KdlName("c17")] C17,
}

public enum ECppDialect
{
    [KdlName("default")] Default,
    [KdlName("c++98")] Cpp98,
    [KdlName("c++11")] Cpp11,
    [KdlName("c++14")] Cpp14,
    [KdlName("c++17")] Cpp17,
    [KdlName("c++20")] Cpp20,
}

public enum ECSharpDialect
{
    [KdlName("default")] Default,
    [KdlName("c#8")] CSharp8,
    [KdlName("c#9")] CSharp9,
    [KdlName("c#10")] CSharp10,
    [KdlName("c#11")] CSharp11,
    [KdlName("c#12")] CSharp12,
}

public class DialectNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "dialect";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
        ModuleNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "c", NodeKdlEnum<ECDialect>.Optional },
        { "cpp", NodeKdlEnum<ECppDialect>.Optional },
        { "csharp", NodeKdlEnum<ECSharpDialect>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public ECDialect CDialect => GetEnumValue("c", ECDialect.Default);
    public ECppDialect CppDialect => GetEnumValue("cpp", ECppDialect.Default);
    public ECSharpDialect CSharpDialect => GetEnumValue("csharp", ECSharpDialect.Default);
}
