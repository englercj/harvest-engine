// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EWhenMode
{
    [KdlName("all")] And,
    [KdlName("any")] Or,
    [KdlName("one")] Xor,
}

public class WhenNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "when";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        InstallNode.NodeName,
        ModuleNode.NodeName,
        PluginNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EWhenMode>.Optional(EWhenMode.And),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "arch", NodeKdlString.Optional() },
        { "configuration", NodeKdlString.Optional() },
        { "host", NodeKdlString.Optional() },
        { "language", NodeKdlString.Optional() },
        { "option", NodeKdlString.Optional() },
        { "platform", NodeKdlString.Optional() },
        { "system", NodeKdlString.Optional() },
        { "tags", NodeKdlString.Optional() },
        { "toolset", NodeKdlString.Optional() },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

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

        CheckValueEquals(conditions, Arch, context.Platform?.Arch ?? EPlatformArch.X86_64);
        CheckValueEquals(conditions, Configuration, context.Configuration?.ConfigName ?? "");
        CheckValueEquals(conditions, Host, context.Host);
        CheckValueEquals(conditions, Language, context.Module?.Language ?? EModuleLanguage.Cpp);
        CheckValueActive(conditions, Option, context.Options);
        CheckValueEquals(conditions, Platform, context.Platform?.PlatformName ?? "");
        CheckValueEquals(conditions, System, context.Platform?.System ?? EPlatformSystem.Windows);
        CheckValueActive(conditions, Tags, context.Tags);
        CheckValueEquals(conditions, Toolset, context.Platform?.Toolset ?? EToolset.MSVC);

        switch (Mode)
        {
            case EWhenMode.And:
                return conditions.All(x => x);
            case EWhenMode.Or:
                return conditions.Any(x => x);
            case EWhenMode.Xor:
                return conditions.Count(x => x) == 1;
        }

        return false;
    }

    private static void CheckValueEquals(List<bool> conditions, string? expr, string value)
    {
        if (string.IsNullOrEmpty(expr))
            return;

        bool result = WhenExpressionEvaluator.Evaluate(expr, (v) =>
        {
            return v == value;
        });
        conditions.Add(result);
    }

    private static void CheckValueEquals<T>(List<bool> conditions, string? expr, T value) where T : struct, Enum
    {
        if (string.IsNullOrEmpty(expr))
            return;

        bool result = WhenExpressionEvaluator.Evaluate(expr, (v) =>
        {
            if (KdlEnumUtils.TryParse(v, out T enumValue))
            {
                return Equals(enumValue, value);
            }
            return false;
        });
        conditions.Add(result);
    }

    private static void CheckValueActive(List<bool> conditions, string? expr, HashSet<string> active)
    {
        if (string.IsNullOrEmpty(expr))
            return;

        bool result = WhenExpressionEvaluator.Evaluate(expr, active.Contains);
        conditions.Add(result);
    }

    private static void CheckValueActive(List<bool> conditions, string? expr, SortedDictionary<string, object?> active)
    {
        if (string.IsNullOrEmpty(expr))
            return;

        bool result = WhenExpressionEvaluator.Evaluate(expr, (v) =>
        {
            if (string.IsNullOrEmpty(v))
                return false;

            string[] parts = v.Split(':');
            if (active.TryGetValue(parts[0], out object? optionValue))
            {
                if (parts.Length == 1)
                    return true;

                return optionValue switch
                {
                    string s => s == parts[1],
                    int i => i == int.Parse(parts[1]),
                    float f => f == float.Parse(parts[1]),
                    bool b => b == bool.Parse(parts[1]),
                    _ => false,
                };
            }

            return false;
        });
        conditions.Add(result);
    }
}
