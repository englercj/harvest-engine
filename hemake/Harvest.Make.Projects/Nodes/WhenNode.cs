// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;
using System.Diagnostics;

namespace Harvest.Make.Projects.Nodes;

public enum EWhenMode
{
    [KdlName("all")] All,
    [KdlName("any")] Any,
    [KdlName("one")] One,
}

public class WhenNodeTraits : NodeBaseTraits
{
    public override string Name => "when";

    public override IReadOnlyList<string> ValidScopes =>
    [
        InstallNode.NodeTraits.Name,
        ModuleNode.NodeTraits.Name,
        PluginNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EWhenMode>.Optional(EWhenMode.All),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "option", NodeValueDef_String.Optional() },
        { "tags", NodeValueDef_String.Optional() },
    };

    public override INode CreateNode(KdlNode node) => new WhenNode(node);

    protected override void ValidateProperties(KdlNode node)
    {
        base.ValidateProperties(node);

        foreach ((string key, KdlValue value) in node.Properties)
        {
            if (value is not KdlString)
            {
                throw new NodeParseException(node, $"'{node.Name}' node has incorrect value type in property '{key}': {value.GetType().Name}. Expected KdlString.");
            }

            if (!PropertyDefs.ContainsKey(key) && !key.Contains('.'))
            {
                throw new NodeParseException(node, $"Invalid when property '{key}'. Unknown built-in condition key; use a token-style key like 'module.kind'.");
            }
        }
    }

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        Debug.Assert(source.Name == Name);

        KdlNode resolvedWhenNode = resolver.CreateResolvedNode(source, includeChildren: false);
        WhenNode when = new(resolvedWhenNode);
        if (when.IsActive(resolver.ProjectContext, resolver.IndexedNodes))
        {
            // Resolve children from the original source when-node so active blocks emit content.
            resolver.ResolveNodeChildren(target, source, WhenNode.NodeTraits, replacer);
        }

        // We return true to indicate that we handled the resolution of this node. However, we do not
        // emit a resolved node here because the when node itself is not in the resolved tree.
        // Instead, we will emit the children of the when node if they are active by recursing into
        // the resolver.ResolveNodeChildren call above.
        resolvedNode = null;
        return true;
    }
}

public class WhenNode(KdlNode node) : NodeBase<WhenNodeTraits>(node)
{
    public EWhenMode Mode => GetEnumValue<EWhenMode>(0);

    public bool IsActive(ProjectContext context, IndexedNodeCollection indexedNodes)
    {
        List<bool> conditions = [];
        conditions.Capacity = 16;

        foreach ((string key, KdlValue value) in Node.Properties)
        {
            string expression = ((KdlString)value).Value;

            switch (key)
            {
                case "option":
                    conditions.Add(WhenExpressionEvaluator.Evaluate(expression, context.Options));
                    break;
                case "tags":
                    conditions.Add(WhenExpressionEvaluator.Evaluate(expression, context.Tags));
                    break;
                default:
                    conditions.Add(EvaluateTokenCondition(context, indexedNodes, key, expression));
                    break;
            }
        }

        return Mode switch
        {
            EWhenMode.All => conditions.All(x => x),
            EWhenMode.Any => conditions.Any(x => x),
            EWhenMode.One => conditions.Count(x => x) == 1,
            _ => false,
        };
    }

    private bool EvaluateTokenCondition(ProjectContext context, IndexedNodeCollection indexedNodes, string key, string expression)
    {
        NodeTokenHandler handler = new(context, indexedNodes, Node);
        StringTokenReplacer replacer = new(handler);
        string tokenValue = replacer.ReplaceTokens($"${{{key}}}");
        return WhenExpressionEvaluator.Evaluate(expression, tokenValue);
    }
}
