// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum ESetAction
{
    [KdlName("add")] Add,
    [KdlName("remove")] Remove,
    [KdlName("update")] Update,
}

public abstract class NodeSetBaseTraits<TChild> : NodeBaseTraits
    where TChild : INode
{
    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<ESetAction>.Optional(ESetAction.Add),
    ];

    public override Type? ChildNodeType => typeof(TChild);
}

public abstract class NodeSetBase<TTraits, TChild>(KdlNode node, INode? scope) : NodeBase<TTraits>(node, scope)
    where TTraits : NodeSetBaseTraits<TChild>, new()
    where TChild : INode
{
    public ESetAction SetAction => GetEnumValue<ESetAction>(0);
    public IEnumerable<TChild> Entries => Children.Cast<TChild>();

    protected readonly Dictionary<string, TChild> _resolvedEntries = [];

    protected override void MergeAndResolveChildren(ProjectContext context, INode node)
    {
        if (node is not NodeSetBase<TTraits, TChild> set)
        {
            throw new Exception($"Cannot merge and resolve children of different node types: {GetType().Name} and {node.GetType().Name}");
        }

        switch (set.SetAction)
        {
            case ESetAction.Add:
            {
                foreach (TChild entry in set.Entries)
                {
                    string key = GetSetEntryKey(context, entry);
                    if (_resolvedEntries.TryGetValue(key, out TChild? existing))
                    {
                        OnModifyChild(context, entry, existing);
                    }
                    else
                    {
                        _resolvedEntries.Add(key, entry);
                        OnAddChild(context, entry);
                    }
                }
                break;
            }
            case ESetAction.Remove:
            {
                foreach (TChild entry in set.Entries)
                {
                    string key = GetSetEntryKey(context, entry);
                    if (_resolvedEntries.TryGetValue(key, out TChild? existing))
                    {
                        _resolvedEntries.Remove(key);
                        OnRemoveChild(context, existing);
                    }
                }
                break;
            }
            case ESetAction.Update:
            {
                foreach (TChild entry in set.Entries)
                {
                    string key = GetSetEntryKey(context, entry);
                    if (_resolvedEntries.TryGetValue(key, out TChild? existing))
                    {
                        OnModifyChild(context, entry, existing);
                    }
                }
                break;
            }
        }
    }

    protected virtual string GetSetEntryKey(ProjectContext context, TChild entry)
    {
        return entry.Node.Name;
    }

    protected virtual void OnAddChild(ProjectContext context, TChild entry)
    {
        Children.Add(entry);
    }

    protected virtual void OnRemoveChild(ProjectContext context, TChild entry)
    {
        Children.Remove(entry);
    }

    protected virtual void OnModifyChild(ProjectContext context, TChild entry, TChild existing)
    {
        existing.MergeAndResolve(context, entry);
    }
}
