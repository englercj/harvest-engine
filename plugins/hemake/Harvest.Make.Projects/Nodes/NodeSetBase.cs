// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum ESetAction
{
    [KdlName("add")] Add,
    [KdlName("remove")] Remove,
    [KdlName("modify")] Modify,
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
                    if (_resolvedEntries.TryGetValue(entry.Node.Name, out T? existing))
                    {
                        existing.MergeAndResolve(context, entry);
                    }
                    else
                    {
                        _resolvedEntries.Add(entry.Node.Name, entry);
                        Children.Add(entry);
                    }
                }
                break;
            }
            case ESetAction.Remove:
            {
                foreach (T entry in set.Entries)
                {
                    if (_resolvedEntries.TryGetValue(entry.Node.Name, out T? existing))
                    {
                        _resolvedEntries.Remove(entry.Node.Name);
                        Children.Remove(existing);
                    }
                }
                break;
            }
            case ESetAction.Modify:
            {
                foreach (T entry in set.Entries)
                {
                    if (_resolvedEntries.TryGetValue(entry.Node.Name, out T? existing))
                    {
                        existing.MergeAndResolve(context, entry);
                    }
                }
                break;
            }
        }
    }
}
