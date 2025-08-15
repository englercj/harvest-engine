// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EWhenMode
{
    [KdlName("all")] All,
    [KdlName("any")] Any,
    [KdlName("one")] One,
}

public class WhenNodeTraits : NodeBaseTraits
{
    public override string Name => "when";

    public override IReadOnlyList<string> ValidScopes =>
    [
        InstallNode.NodeTraits.Name,
        ModuleNode.NodeTraits.Name,
        PluginNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EWhenMode>.Optional(EWhenMode.All),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "arch", NodeValueDef_String.Optional() },
        { "configuration", NodeValueDef_String.Optional() },
        { "host", NodeValueDef_String.Optional() },
        { "language", NodeValueDef_String.Optional() },
        { "option", NodeValueDef_String.Optional() },
        { "platform", NodeValueDef_String.Optional() },
        { "system", NodeValueDef_String.Optional() },
        { "tags", NodeValueDef_String.Optional() },
        { "toolset", NodeValueDef_String.Optional() },
    };
}

public class WhenNode(KdlNode node, INode? scope) : NodeBase<WhenNodeTraits>(node, scope)
{
    public EWhenMode Mode => GetEnumValue<EWhenMode>(0);
    public string? Arch => TryGetStringValue("arch");
    public string? Configuration => TryGetStringValue("configuration");
    public string? Host => TryGetStringValue("host");
    public string? Language => TryGetStringValue("language");
    public string? Option => TryGetStringValue("option");
    public string? Platform => TryGetStringValue("platform");
    public string? System => TryGetStringValue("system");
    public string? Tags => TryGetStringValue("tags");
    public string? Toolset => TryGetStringValue("toolset");

    public bool IsActive(ProjectContext context)
    {
        List<bool> conditions = [];
        conditions.Capacity = 16;

        CheckValueActive(conditions, Arch, context.Platform?.Arch ?? EPlatformArch.X86_64);
        CheckValueActive(conditions, Configuration, context.Configuration?.ConfigName ?? "");
        CheckValueActive(conditions, Host, context.Host);
        CheckValueActive(conditions, Language, context.Module?.Language ?? EModuleLanguage.Cpp);
        CheckValueActive(conditions, Option, context.Options);
        CheckValueActive(conditions, Platform, context.Platform?.PlatformName ?? "");
        CheckValueActive(conditions, System, context.Platform?.System ?? EPlatformSystem.Windows);
        CheckValueActive(conditions, Tags, context.Tags);
        CheckValueActive(conditions, Toolset, context.Platform?.Toolset ?? EToolset.MSVC);

        return Mode switch
        {
            EWhenMode.All => conditions.All(x => x),
            EWhenMode.Any => conditions.Any(x => x),
            EWhenMode.One => conditions.Count(x => x) == 1,
            _ => false,
        };
    }

    private static void CheckValueActive<T>(List<bool> conditions, string? expr, T value)
    {
        if (string.IsNullOrEmpty(expr))
            return;

        conditions.Add(WhenExpressionEvaluator.Evaluate(expr, value));
    }
}
