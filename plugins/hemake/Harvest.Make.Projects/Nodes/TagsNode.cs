// Copyright Chad Engler

using Harvest.Kdl;
using System.Diagnostics;

namespace Harvest.Make.Projects.Nodes;

internal class TagsNodeTraits : NodeSetBaseTraits<TagsEntryNode>
{
    public override string Name => "tags";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override INode CreateNode(KdlNode node) => new TagsNode(node);

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        Debug.Assert(source.Name == Name);

        resolvedNode = resolver.CreateResolvedNode(source);

        TagsNode tags = new(resolvedNode);
        foreach (TagsEntryNode entry in tags.Entries)
        {
            resolver.AddActiveTagForScope(target, entry.TagName);
        }

        return true;
    }
}

internal class TagsNode(KdlNode node) : NodeSetBase<TagsNodeTraits, TagsEntryNode>(node)
{
}
