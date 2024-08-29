// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class BuildOutputNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "build_output";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlValue<KdlString>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "bin_dir", NodeKdlValue<KdlString>.Optional },
        { "gen_dir", NodeKdlValue<KdlString>.Optional },
        { "lib_dir", NodeKdlValue<KdlString>.Optional },
        { "obj_dir", NodeKdlValue<KdlString>.Optional },
        { "plugin_dir", NodeKdlValue<KdlString>.Optional },
        { "project_dir", NodeKdlValue<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
    public override Type? ChildNodeType => typeof(BuildOptionsEntryNode);

    public string BasePath => GetStringValue(0) ?? "build";

    public string BinDir => GetStringValue("bin_dir") ?? "${platform.name:lower}-${configuration.name:lower}/bin";
    public string GenDir => GetStringValue("gen_dir") ?? "${platform.name:lower}-${configuration.name:lower}/generated/${module.name}";
    public string LibDir => GetStringValue("lib_dir") ?? "${platform.name:lower}-${configuration.name:lower}/lib/${module.name}";
    public string ObjDir => GetStringValue("obj_dir") ?? "${platform.name:lower}-${configuration.name:lower}/obj/${module.name}";
    public string PluginDir => GetStringValue("plugin_dir") ?? "plugins";
    public string ProjectDir => GetStringValue("project_dir") ?? "projects";
}
