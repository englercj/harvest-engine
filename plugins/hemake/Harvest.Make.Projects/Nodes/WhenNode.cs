// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;
using System.Diagnostics;

namespace Harvest.Make.Projects.Nodes;

public enum EWhenMode
{
    [KdlName("all")] All,
    [KdlName("any")] Any,
    [KdlName("one")] One,
}

internal class WhenNodeTraits : NodeBaseTraits
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

    public override INode CreateNode(KdlNode node) => new WhenNode(node);

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        Debug.Assert(source.Name == Name);

        KdlNode resolvedWhenNode = resolver.CreateResolvedNode(source, includeChildren: false);
        WhenNode when = new(resolvedWhenNode);
        if (when.IsActive(resolver.ProjectContext))
        {
            // Resolve children from the original source when-node so active blocks emit content.
            resolver.ResolveNodeChildren(target, source, WhenNode.NodeTraits, replacer);
        }

        // We return true to indicate that we handled the resolution of this node. However, we do not
        // emit a resolved node here because the when node itself is not in the resolved tree.
        // Instead, we will emit the children of the when node if they are active by recursing into
        // the resolver.ResolveNodeChildren call above.
        resolvedNode = null;
        return true;
    }
}

internal class WhenNode(KdlNode node) : NodeBase<WhenNodeTraits>(node)
{
    public EWhenMode Mode => GetEnumValue<EWhenMode>(0);
    public string? Arch => TryGetValue("arch", out string? value) ? value : null;
    public string? Configuration => TryGetValue("configuration", out string? value) ? value : null;
    public string? Host => TryGetValue("host", out string? value) ? value : null;
    public string? Language => TryGetValue("language", out string? value) ? value : null;
    public string? Option => TryGetValue("option", out string? value) ? value : null;
    public string? Platform => TryGetValue("platform", out string? value) ? value : null;
    public string? System => TryGetValue("system", out string? value) ? value : null;
    public string? Tags => TryGetValue("tags", out string? value) ? value : null;
    public string? Toolset  => TryGetValue("toolset", out string? value) ? value : null;

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
