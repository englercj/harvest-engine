// Copyright Chad Engler

using Harvest.Kdl;
using System.Diagnostics;

namespace Harvest.Make.Projects.Nodes;

public class PluginNodeTraits : NodeBaseTraits
{
    public override string Name => "plugin";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "version", NodeValueDef_String.Required() },
        { "license", NodeValueDef_String.Optional() },
    };

    public override string? TryResolveToken(ProjectContext projectContext, KdlNode contextNode, string propertyName)
    {
        Debug.Assert(contextNode.Name == Name);

        PluginNode plugin = new(contextNode);

        switch (propertyName)
        {
            case "name": return plugin.PluginName;
            case "install_dir": return plugin.GetInstallDir(projectContext);
        }

        return base.TryResolveToken(projectContext, contextNode, propertyName);
    }

    public override INode CreateNode(KdlNode node) => new PluginNode(node);
}

public class PluginNode(KdlNode node) : NodeBase<PluginNodeTraits>(node)
{
    public string PluginName => GetValue<string>(0);
    public string Version => GetValue<string>("version");
    public string? License => TryGetValue("license", out string? value) ? value : null;

    public string GetInstallDir(ProjectContext projectContext)
    {
        List<FetchNode> fetchNodes = projectContext.ProjectService.GetNodes<FetchNode>(projectContext, this, false);
        if (fetchNodes.MaxBy((n) => n.InstallDirPriority) is FetchNode primaryFetchNode)
        {
            string installBaseDir = GetInstallBaseDir(projectContext);
            string baseDir = Path.Combine(installBaseDir, primaryFetchNode.ArchiveKey);
            return Path.Combine(baseDir, primaryFetchNode.ArchiveBaseDir);
        }

        // No fetch nodes, use the node's file directory as the install dir
        return Path.GetDirectoryName(Node.SourceInfo.FilePath) ?? string.Empty;
    }

    private string GetInstallBaseDir(ProjectContext projectContext)
    {
        BuildOutputNode buildOutput = projectContext.ProjectService.GetMergedNode<BuildOutputNode>(projectContext, this, false);
        return buildOutput.InstallDir;
    }
}
