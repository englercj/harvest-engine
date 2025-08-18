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
    where TChild : class, INode
{
    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<ESetAction>.Optional(ESetAction.Add),
    ];

    public override Type? ChildNodeType => typeof(TChild);
}

public abstract class NodeSetBase<TTraits, TChild>(KdlNode node) : NodeBase<TTraits>(node)
    where TTraits : NodeSetBaseTraits<TChild>, new()
    where TChild : class, INode
{
    public ESetAction SetAction => GetEnumValue<ESetAction>(0);

    public IEnumerable<TChild> Entries =>
        Node.Children.Select(child => Activator.CreateInstance(typeof(TChild), child) as TChild
            ?? throw new NodeParseException(child, $"Activator.CreateInstance failed to allocate node instance for type '{typeof(TChild)}'."));

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

    protected virtual void OnModifyChild(ProjectContext context, TChild entry, TChild existing)
    {
        existing.MergeAndResolve(context, entry);
    }
}
