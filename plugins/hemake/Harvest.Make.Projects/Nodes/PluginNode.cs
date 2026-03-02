// Copyright Chad Engler

using Harvest.Kdl;
using System.Diagnostics;

namespace Harvest.Make.Projects.Nodes;

internal class PluginNodeTraits : NodeBaseTraits
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
        { "license", NodeValueDef_String.Optional("UNLICENSED") },
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

internal class PluginNode(KdlNode node) : NodeBase<PluginNodeTraits>(node)
{
    public string PluginName => GetValue<string>(0);
    public string Version => GetValue<string>("version");
    public string License => GetValue<string>("license");

    public string GetInstallDir(ProjectContext projectContext)
    {
        List<FetchNode> fetchNodes =
        [
            .. Node
                .GetDescendantsByName(FetchNode.NodeTraits.Name)
                .Select((fetchNode) => new FetchNode(fetchNode))
        ];

        if (fetchNodes.MaxBy((n) => n.InstallDirPriority) is FetchNode primaryFetchNode)
        {
            string installBaseDir = GetInstallBaseDir(projectContext);
            string baseDir = Path.Combine(installBaseDir, primaryFetchNode.ArchiveKey);
            return Path.Combine(baseDir, primaryFetchNode.ArchiveBaseDir);
        }

        // No fetch nodes, use the node's file directory as the install dir
        return Path.GetDirectoryName(Node.SourceInfo.FilePath) ?? "";
    }

    private string GetInstallBaseDir(ProjectContext projectContext)
    {
        KdlNode? scope = Node;
        while (scope is not null && scope.Name != ProjectNode.NodeTraits.Name)
        {
            scope = scope.Parent;
        }

        if (scope is null)
        {
            throw new InvalidOperationException("Plugin node is not within a project scope.");
        }

        ProjectNode project = new(scope);
        return project.InstallsDir;
    }
}
