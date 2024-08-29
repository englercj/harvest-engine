// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum ECodegenMode
{
    [KdlName("default")] Default,
    [KdlName("avx")] AVX,
    [KdlName("avx2")] AVX2,
    [KdlName("avx512")] AVX512,
    [KdlName("sse")] SSE,
    [KdlName("sse2")] SSE2,
    [KdlName("sse3")] SSE3,
    [KdlName("ssse3")] SSSE3,
    [KdlName("sse4.1")] SSE41,
    [KdlName("sse4.2")] SSE42,
    [KdlName("simd128")] Simd128,
    [KdlName("armv8.0")] ARMv8_0,
    [KdlName("armv8.1")] ARMv8_1,
    [KdlName("armv8.2")] ARMv8_2,
    [KdlName("armv8.3")] ARMv8_3,
    [KdlName("armv8.4")] ARMv8_4,
    [KdlName("armv8.5")] ARMv8_5,
    [KdlName("armv8.6")] ARMv8_6,
    [KdlName("armv8.7")] ARMv8_7,
    [KdlName("armv8.8")] ARMv8_8,
}

public class CodegenNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "codegen";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
        ModuleNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<ECodegenMode>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public ECodegenMode CodegenMode => GetEnumValue(0, ECodegenMode.AVX);
}
