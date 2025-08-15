// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class PluginNode(KdlNode node, INode? scope) : NodeBase<PluginNode>(node, scope)
{
    public static string NodeName => "plugin";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "version", NodeValueDef_String.Required() },
        { "license", NodeValueDef_String.Optional() },
    };

    public string PluginName => GetStringValue(0);
    public string Version => GetStringValue("version");
    public string? License => TryGetStringValue("license");

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
