// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;
using static System.Formats.Asn1.AsnWriter;

namespace Harvest.Make.Projects;

public class NodeTraits<T> where T : class, INode
{
    private static readonly T s_instance = (T)Activator.CreateInstance(typeof(T), new KdlNode(""), null)!;

    public static T CreateInstance(INode? scope = null)
    {
        return Activator.CreateInstance(typeof(T), new KdlNode(Name), scope) as T
            ?? throw new Exception($"Failed to allocate resolved node {Name}.");
    }

    public static string Name => s_instance.Name;

    public static bool IsExtensionNode => s_instance.IsExtensionNode;
    public static bool CanBeExtended => s_instance.CanBeExtended;
    public static ENodeDependencyInheritance DependencyInheritance => s_instance.DependencyInheritance;

    public static IReadOnlyList<string> Scopes => s_instance.Scopes;
    public static IReadOnlyList<NodeKdlValue> Arguments => s_instance.Arguments;
    public static IReadOnlyDictionary<string, NodeKdlValue> Properties => s_instance.Properties;
    public static INode? Scope => s_instance.Scope;
    public static List<INode> Children => s_instance.Children;
    public static Type? ChildNodeType => s_instance.ChildNodeType;
}
