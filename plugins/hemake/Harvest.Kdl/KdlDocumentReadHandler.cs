// Copyright Chad Engler

namespace Harvest.Kdl;

internal class KdlDocumentReadHandler : IKdlReadHandler
{
    public KdlDocument Document = new();

    private List<KdlNode> _nodeStack = new();
    private uint _commentDepth = 0;

    public bool StartDocument()
    {
        // Nothing to do here
        return true;
    }

    public bool EndDocument()
    {
        // Nothing to do here
        return true;
    }

    public bool Comment(string value)
    {
        // Nothing to do here
        return true;
    }

    public bool StartComment()
    {
        ++_commentDepth;
        return true;
    }

    public bool EndComment()
    {
        --_commentDepth;
        return true;
    }

    public bool StartNode(string name, string? type)
    {
        if (_commentDepth > 0)
            return true;

        IList<KdlNode> children = _nodeStack.Count == 0 ? Document.Nodes : _nodeStack[^1].Children;
        KdlNode node = new(name, type);
        children.Add(node);
        _nodeStack.Add(node);
        return true;
    }

    public bool EndNode()
    {
        if (_commentDepth > 0)
            return true;

        if (_nodeStack.Count == 0)
            return false;

        _nodeStack.RemoveAt(_nodeStack.Count - 1);
        return true;
    }

    public bool Argument(KdlValue value)
    {
        if (_commentDepth > 0)
            return true;

        if (_nodeStack.Count == 0)
            return false;

        KdlNode node = _nodeStack[^1];
        node.Arguments.Add(value);
        return true;
    }

    public bool Property(string name, KdlValue value)
    {
        if (_commentDepth > 0)
            return true;

        if (_nodeStack.Count == 0)
            return false;

        KdlNode node = _nodeStack[^1];
        node.Properties[name] = value;
        return true;
    }
}
