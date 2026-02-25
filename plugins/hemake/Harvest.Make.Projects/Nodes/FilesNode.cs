// Copyright Chad Engler

using Harvest.Kdl;
using System.Diagnostics;

namespace Harvest.Make.Projects.Nodes;

public class FilesNodeTraits : NodeSetBaseTraits<FilesEntryNode>
{
    public override string Name => "files";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        Debug.Assert(source.Name == Name);

        resolvedNode = resolver.CreateResolvedNode(source, includeChildren: false);

        Dictionary<string, KdlNode> resolvedPaths = [];

        foreach (KdlNode entry in source.Children)
        {
            KdlNode resolvedEntry = resolver.CreateResolvedNode(entry, includeChildren: false);
            string directory = Path.GetDirectoryName(resolvedEntry.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();
            IReadOnlyList<string> filePaths = resolver.ProjectContext.ProjectService.PathGlobs.ExpandPath(resolvedEntry.Name, directory);

            foreach (string filePath in filePaths)
            {
                if (resolvedPaths.TryGetValue(filePath, out KdlNode? existing))
                {
                    resolvedEntry.CopyTo(existing, includeChildren: false);
                }
                else
                {
                    resolvedPaths.Add(filePath, resolvedEntry);
                    resolvedNode.AddChild(resolvedEntry);
                }
            }
        }

        return true;
    }

    public override INode CreateNode(KdlNode node) => new FilesNode(node);
}

public class FilesNode(KdlNode node) : NodeSetBase<FilesNodeTraits, FilesEntryNode>(node)
{
}
