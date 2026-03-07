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

            // Glob expansion will only return "real" file paths that it can find on disk. If the
            // user-specified path doesn't contain any glob patterns, or if it contains tokens that
            // can't be resolved to real paths during generation, we should treat the original path
            // as a literal path to include, even if it doesn't exist on disk. This allows for more
            // flexible scenarios where generated projects might reference files that only exist in
            // the context of the generated project. For example, this allows the user to do
            // reference files like "$(WindowsSdkDir)Redist/D3D/x64/dxcompiler.dll".
            bool hasGlobPattern = resolvedEntry.Name.IndexOfAny(['*', '?']) >= 0;
            if (filePaths.Count == 0 && (resolvedEntry.Name.Contains("$(") || !hasGlobPattern))
            {
                filePaths = [resolvedEntry.Name];
            }

            foreach (string filePath in filePaths)
            {
                if (resolvedPaths.TryGetValue(filePath, out KdlNode? existing))
                {
                    resolvedEntry.CopyTo(existing, includeChildren: false);
                }
                else
                {
                    KdlNode expandedEntry = new(filePath, resolvedEntry.Type)
                    {
                        SourceInfo = resolvedEntry.SourceInfo,
                    };
                    resolvedEntry.CopyTo(expandedEntry, includeChildren: false);

                    resolvedPaths.Add(filePath, expandedEntry);
                    resolvedNode.AddChild(expandedEntry);
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
