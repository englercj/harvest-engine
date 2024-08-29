// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EBuildEvent
{
    [KdlName("prebuild")] Prebuild,
    [KdlName("postbuild")] Postbuild,
    [KdlName("prelink")] Prelink,
}

public class BuildEventNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "build_event";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EBuildEvent>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "message", NodeKdlValue<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
    public override Type? ChildNodeType => typeof(BuildEventEntryNode);

    public EBuildEvent EventName => GetEnumValue(0, EBuildEvent.Prebuild);

    public string? Message => GetStringValue("message");

    public IEnumerable<BuildEventEntryNode> Entries
    {
        get
        {
            foreach (INode child in Children)
            {
                yield return (BuildEventEntryNode)child;
            }
        }
    }
}
