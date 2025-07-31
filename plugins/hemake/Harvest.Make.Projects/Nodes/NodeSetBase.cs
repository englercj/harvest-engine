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

public abstract class NodeSetBase<T>(KdlNode node, INode? scope) : NodeBase(node, scope) where T : INode
{
    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<ESetAction>.Optional(ESetAction.Add),
    ];

    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override Type? ChildNodeType => typeof(T);

    public ESetAction SetAction => GetEnumValue<ESetAction>(0);
    public IEnumerable<T> Entries => Children.Cast<T>();

    protected readonly Dictionary<string, T> _resolvedEntries = [];

    protected override void MergeAndResolveChildren(ProjectContext context, INode node)
    {
        if (node is not NodeSetBase<T> set)
        {
            throw new Exception($"Cannot merge and resolve children of different node types: {GetType().Name} and {node.GetType().Name}");
        }

        switch (set.SetAction)
        {
            case ESetAction.Add:
            {
                foreach (T entry in set.Entries)
                {
                    string key = GetSetEntryKey(context, entry);
                    if (_resolvedEntries.TryGetValue(key, out T? existing))
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
                foreach (T entry in set.Entries)
                {
                    string key = GetSetEntryKey(context, entry);
                    if (_resolvedEntries.TryGetValue(key, out T? existing))
                    {
                        _resolvedEntries.Remove(key);
                        OnRemoveChild(context, existing);
                    }
                }
                break;
            }
            case ESetAction.Update:
            {
                foreach (T entry in set.Entries)
                {
                    string key = GetSetEntryKey(context, entry);
                    if (_resolvedEntries.TryGetValue(key, out T? existing))
                    {
                        OnModifyChild(context, entry, existing);
                    }
                }
                break;
            }
        }
    }

    protected virtual string GetSetEntryKey(ProjectContext context, T entry)
    {
        return entry.Node.Name;
    }

    protected virtual void OnAddChild(ProjectContext context, T entry)
    {
        Children.Add(entry);
    }

    protected virtual void OnRemoveChild(ProjectContext context, T entry)
    {
        Children.Remove(entry);
    }

    protected virtual void OnModifyChild(ProjectContext context, T entry, T existing)
    {
        existing.MergeAndResolve(context, entry);
    }
}
