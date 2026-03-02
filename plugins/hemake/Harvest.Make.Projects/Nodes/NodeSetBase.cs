// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;
using System.Xml.Linq;

namespace Harvest.Make.Projects.Nodes;

public enum ESetAction
{
    [KdlName("add")] Add,
    [KdlName("remove")] Remove,
    [KdlName("update")] Update,
}

internal abstract class NodeSetBaseTraits<TChild> : NodeBaseTraits
    where TChild : class, INode
{
    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<ESetAction>.Optional(ESetAction.Add),
    ];

    public override INodeTraits? ChildNodeTraits => TChild.NodeTraits;
}

internal abstract class NodeSetBase<TTraits, TChild>(KdlNode node) : NodeBase<TTraits>(node)
    where TTraits : NodeSetBaseTraits<TChild>, new()
    where TChild : class, INode
{
    public ESetAction SetAction => GetEnumValue<ESetAction>(0);

    public IEnumerable<TChild> Entries => Node.Children.Select(INodeTraits.CreateNode<TChild>);

    protected readonly Dictionary<string, KdlNode> _resolvedEntries = [];
    protected virtual bool CloneSetEntryChildrenWhenMerging => false;

    public override void MergeNode(ProjectContext projectContext, KdlNode node)
    {
        if (Node.Name != node.Name)
        {
            throw new ArgumentException("Cannot merge nodes of different types.", nameof(node));
        }

        node.CopyTo(Node, includeChildren: false);

        ESetAction setAction = Traits.GetEnumValue<ESetAction>(node, 0);
        switch (setAction)
        {
            case ESetAction.Add:
            {
                foreach (KdlNode entry in node.Children)
                {
                    string key = GetSetEntryKey(projectContext, entry);
                    if (_resolvedEntries.TryGetValue(key, out KdlNode? existing))
                    {
                        entry.CopyTo(existing, CloneSetEntryChildrenWhenMerging);
                    }
                    else
                    {
                        KdlNode clone = entry.Clone(CloneSetEntryChildrenWhenMerging);
                        _resolvedEntries.Add(key, clone);
                        Node.AddChild(clone);
                    }
                }
                break;
            }
            case ESetAction.Remove:
            {
                foreach (KdlNode entry in node.Children)
                {
                    string key = GetSetEntryKey(projectContext, entry);
                    if (_resolvedEntries.TryGetValue(key, out KdlNode? existing))
                    {
                        _resolvedEntries.Remove(key);
                        existing.RemoveFromParent();
                    }
                }
                break;
            }
            case ESetAction.Update:
            {
                foreach (KdlNode entry in node.Children)
                {
                    string key = GetSetEntryKey(projectContext, entry);
                    if (_resolvedEntries.TryGetValue(key, out KdlNode? existing))
                    {
                        entry.CopyTo(existing, CloneSetEntryChildrenWhenMerging);
                    }
                }
                break;
            }
        }
    }

    protected virtual string GetSetEntryKey(ProjectContext context, KdlNode entry)
    {
        return entry.Name;
    }
}
