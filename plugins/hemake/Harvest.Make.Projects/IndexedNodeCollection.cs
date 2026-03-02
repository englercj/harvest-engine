// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects;

internal class IndexedNodeCollection
{
    private readonly Dictionary<string, SortedDictionary<string, KdlNode>> _indexedNodesByType = [];

    public IReadOnlyDictionary<string, KdlNode> this[string nodeTypeName] => _indexedNodesByType[nodeTypeName];
    public KdlNode this[string nodeTypeName, string nodeId] => _indexedNodesByType[nodeTypeName][nodeId];

    public IEnumerable<KdlNode> GetAllNodes(string nodeTypeName)
    {
        if (_indexedNodesByType.TryGetValue(nodeTypeName, out SortedDictionary<string, KdlNode>? nodes))
        {
            return nodes.Values;
        }

        return [];
    }

    public IEnumerable<T> GetAllNodes<T>() where T : class, INode
    {
        if (_indexedNodesByType.TryGetValue(T.NodeTraits.Name, out SortedDictionary<string, KdlNode>? nodes))
        {
            foreach (KdlNode node in nodes.Values)
            {
                yield return INodeTraits.CreateNode<T>(node);
            }
        }
    }

    public bool TryGetNode(string nodeTypeName, string nodeId, [MaybeNullWhen(false)] out KdlNode node)
    {
        if (_indexedNodesByType.TryGetValue(nodeTypeName, out SortedDictionary<string, KdlNode>? indexedNodes))
        {
            return indexedNodes.TryGetValue(nodeId, out node);
        }

        node = default;
        return false;
    }

    public bool TryGetNode<T>(string nodeId, [NotNullWhen(true)] out T? node) where T : class, INode
    {
        if (TryGetNode(T.NodeTraits.Name, nodeId, out KdlNode? kdlNode))
        {
            node = INodeTraits.CreateNode<T>(kdlNode);
            return true;
        }

        node = default;
        return false;
    }

    public KdlNode GetNode(string nodeTypeName, string nodeId)
    {
        if (TryGetNode(nodeTypeName, nodeId, out KdlNode? node))
        {
            return node;
        }
        throw new KeyNotFoundException($"Node of type '{nodeTypeName}' with ID '{nodeId}' not found.");
    }

    public T GetNode<T>(string nodeId) where T : class, INode
    {
        KdlNode node = GetNode(T.NodeTraits.Name, nodeId);
        return INodeTraits.CreateNode<T>(node);
    }

    public bool TryAddNode(string nodeTypeName, string nodeId, KdlNode node)
    {
        if (!_indexedNodesByType.TryGetValue(nodeTypeName, out SortedDictionary<string, KdlNode>? indexedNodes))
        {
            indexedNodes = [];
            _indexedNodesByType.Add(nodeTypeName, indexedNodes);
        }

        return indexedNodes.TryAdd(nodeId, node);
    }

    public bool TryAddNode<T>(string nodeId, T node) where T : class, INode
    {
        return TryAddNode(T.NodeTraits.Name, nodeId, node.Node);
    }
}
