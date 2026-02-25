// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class NodeTokenHandler(ProjectContext projectContext, IndexedNodeCollection indexedNodes, KdlNode scope) : IStringTokenHandler
{
    // TODO: Remove project context when we're operating on fully resolved nodes only
    private readonly ProjectContext _projectContext = projectContext;
    private readonly IndexedNodeCollection _indexedNodes = indexedNodes;
    private readonly KdlNode _scope = scope;

    public string GetTokenValue(StringTokenContext tokenContext)
    {
        string tokenValue = GetTokenString(tokenContext);

        foreach (string transformerName in tokenContext.TransformerNames)
        {
            if (!_projectContext.ProjectService.TokenTransformers.TryGetValue(transformerName, out CustomStringTokenTransformer? transformer))
            {
                throw new NodeParseException(_scope, $"Invalid token '{tokenContext.Token}'. Unknown transformer '{transformerName}'.");
            }

            if (transformer(tokenValue) is not string transformedValue)
            {
                throw new NodeParseException(_scope, $"Invalid token '{tokenContext.Token}'. Transformer '{transformerName}' failed to transform the token value.");
            }

            tokenValue = transformedValue;
        }

        return tokenValue;
    }

    private string GetTokenString(StringTokenContext tokenContext)
    {
        bool hasContextId = !string.IsNullOrEmpty(tokenContext.ContextId);

        // Check for a custom resolver for this context/property
        if (!hasContextId && _projectContext.ProjectService.TokenResolvers.TryGetValue((tokenContext.ContextName, tokenContext.PropertyName), out CustomStringTokenResolver? resolver))
        {
            if (resolver(_projectContext) is string customResolvedValue)
            {
                return customResolvedValue;
            }

            throw new NodeParseException(_scope, $"Invalid token '{tokenContext.Token}'. Custom resolver for context '{tokenContext.ContextName}' and property '{tokenContext.PropertyName}' failed to resolve the token.");
        }

        // Resolve the context node so we can read properties directly from it
        KdlNode contextNode = hasContextId
            ? GetTokenContextById(_scope, tokenContext.Token, tokenContext.ContextName, tokenContext.ContextId)
            : GetTokenContextByName(_scope, tokenContext.Token, tokenContext.ContextName);

        // Let the node traits try to handle resolving the token
        INodeTraits contextNodeTraits = _projectContext.ProjectService.GetNodeTraits(contextNode);
        if (contextNodeTraits.TryResolveToken(_projectContext, contextNode, tokenContext.PropertyName) is string nodeResolvedValue)
        {
            return nodeResolvedValue;
        }

        // Do generic argument/property resolution
        const string ArgumentKeyPrefix = "_arg";
        if (tokenContext.PropertyName.StartsWith(ArgumentKeyPrefix))
        {
            if (!int.TryParse(tokenContext.PropertyName[ArgumentKeyPrefix.Length..], out int argIndex))
            {
                throw new NodeParseException(_scope, $"Invalid token '{tokenContext.Token}'. Argument index in '{tokenContext.PropertyName}' must be an integer.");
            }

            if (argIndex < 0 && argIndex >= contextNode.Arguments.Count)
            {
                throw new NodeParseException(_scope, $"Invalid token '{tokenContext.Token}'. Argument index {argIndex} is out of range for context '{tokenContext.ContextName}'.");
            }

            return contextNode.Arguments[argIndex].GetValueString();
        }

        if (contextNode.Properties.TryGetValue(tokenContext.PropertyName, out KdlValue? value))
        {
            return value.GetValueString();
        }

        throw new NodeParseException(_scope, $"Invalid token '{tokenContext.Token}'. Unknown property '{tokenContext.PropertyName}' on context '{tokenContext.ContextName}'.");
    }

    private KdlNode GetTokenContextById(KdlNode scope, string token, string contextName, string contextId)
    {
        if (_indexedNodes.TryGetNode(contextName, contextId, out KdlNode? contextNode))
        {
            return contextNode;
        }

        throw new NodeParseException(scope, $"Invalid token '{token}'. No {contextName} node found with name '{contextId}'.");
    }

    private static KdlNode GetTokenContextByName(KdlNode scope, string token, string name)
    {
        KdlNode? search = scope;
        while (search is not null)
        {
            if (search.Name == name)
            {
                return search;
            }

            search = search.Parent;
        }

        throw new NodeParseException(scope, $"Invalid token '{token}'. Failed to find a '{name}' node in the parent heirarchy.");
    }
}
