// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class FilesNodeTraits : NodeSetBaseTraits<FilesEntryNode>
{
    public override string Name => "files";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];
}

public class FilesNode(KdlNode node, INode? scope) : NodeSetBase<FilesNodeTraits, FilesEntryNode>(node, scope)
{
    // Override the merge and resolve logic for children to create a single entry node per
    // expanded file path. This is necessary to support the case where a glob is added, then
    // a more specific glob is removed. The more specific glob may have matched files that
    // were also matched by the less specific glob. If we don't create a single entry node
    // per expanded file path, we won't be able to remove the individual files that were
    // matched by the more specific glob.
    //
    // For example, imagine we first encounter: `files { "src/*.cpp" }` and then we encounter:
    // `files remove { "src/nope.cpp" }`. We need to expand `src/*.cpp` to later match the removed
    // file name.
    protected override void MergeAndResolveChildren(ProjectContext context, INode node)
    {
        if (node is not FilesNode filesNode)
        {
            throw new Exception($"Cannot merge and resolve children of different node types: {GetType().Name} and {node.GetType().Name}");
        }

        switch (filesNode.SetAction)
        {
            case ESetAction.Add:
            {
                foreach (FilesEntryNode entry in filesNode.Entries)
                {
                    foreach (string filePath in entry.FilePaths)
                    {
                        if (_resolvedEntries.TryGetValue(filePath, out FilesEntryNode? existing))
                        {
                            existing.MergeAndResolve(context, entry);
                        }
                        else
                        {
                            KdlNode newKdlNode = new(filePath);
                            FilesEntryNode newEntryNode = new(newKdlNode, this);
                            newEntryNode.MergeAndResolve(context, entry);

                            _resolvedEntries.Add(filePath, newEntryNode);
                            Children.Add(newEntryNode);
                        }
                    }
                }
                break;
            }
            case ESetAction.Remove:
            {
                foreach (FilesEntryNode entry in filesNode.Entries)
                {
                    foreach (string filePath in entry.FilePaths)
                    {
                        if (_resolvedEntries.TryGetValue(filePath, out FilesEntryNode? existing))
                        {
                            _resolvedEntries.Remove(filePath);
                            Children.Remove(existing);
                        }
                    }
                }
                break;
            }
            case ESetAction.Update:
            {
                foreach (FilesEntryNode entry in filesNode.Entries)
                {
                    foreach (string filePath in entry.FilePaths)
                    {
                        if (_resolvedEntries.TryGetValue(filePath, out FilesEntryNode? existing))
                        {
                            existing.MergeAndResolve(context, entry);
                        }
                    }
                }
                break;
            }
        }
    }
}
