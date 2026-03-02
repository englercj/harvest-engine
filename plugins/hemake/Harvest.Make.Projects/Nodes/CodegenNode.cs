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
    [KdlName("armv9.0")] ARMv9_0,
    [KdlName("armv9.1")] ARMv9_1,
    [KdlName("armv9.2")] ARMv9_2,
    [KdlName("armv9.3")] ARMv9_3,
    [KdlName("armv9.4")] ARMv9_4,
}

internal class CodegenNodeTraits : NodeBaseTraits
{
    public override string Name => "codegen";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
        ModuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<ECodegenMode>.Required(ECodegenMode.Default),
    ];

    public override INode CreateNode(KdlNode node) => new CodegenNode(node);
}

internal class CodegenNode(KdlNode node) : NodeBase<CodegenNodeTraits>(node)
{
    public ECodegenMode CodegenMode => GetEnumValue<ECodegenMode>(0);
}
