// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public enum ESetAction
{
    Add,
    Remove,
    Match,
}

public class NodeKdlSetAction : NodeKdlValue<KdlString>
{
    private static List<object> s_validValues = ["add", "remove", "match"];

    public static new NodeKdlSetAction Required => new() { IsRequired = true, ValidValues = s_validValues };
    public static new NodeKdlSetAction Optional => new() { IsRequired = false, ValidValues = s_validValues };
}

public abstract class NodeSetBase<T>(KdlNode node) : NodeBase(node) where T : INode
{
    public override Type? ChildNodeType => typeof(T);

    public ESetAction SetAction => GetStringValue(0) switch
    {
        "remove" => ESetAction.Remove,
        "match" => ESetAction.Match,
        _ => ESetAction.Add,
    };

    public IEnumerable<T> Entries
    {
        get
        {
            foreach (INode child in Children)
            {
                yield return (T)child;
            }
        }
    }
}
