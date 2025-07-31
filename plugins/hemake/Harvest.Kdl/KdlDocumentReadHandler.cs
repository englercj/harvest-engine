// Copyright Chad Engler

namespace Harvest.Kdl;

internal class KdlDocumentReadHandler : IKdlReadHandler
{
    public KdlDocument Document = new();

    private readonly List<KdlNode> _nodeStack = [];
    private uint _commentDepth = 0;

    public void StartDocument(KdlSourceInfo source)
    {
        // Nothing to do here
    }

    public void EndDocument()
    {
        // Nothing to do here
    }

    public void Version(string value, KdlSourceInfo source)
    {
        // Nothing to do here
    }

    public void Comment(string value, KdlSourceInfo source)
    {
        // Nothing to do here
    }

    public void StartComment(KdlSourceInfo source)
    {
        ++_commentDepth;
    }

    public void EndComment()
    {
        --_commentDepth;
    }

    public void StartNode(string name, string? type, KdlSourceInfo source)
    {
        if (_commentDepth > 0)
            return;

        KdlNode node = new(name, type)
        {
            SourceInfo = source
        };

        if (_nodeStack.Count == 0)
        {
            Document.AddChild(node);
        }
        else
        {
            _nodeStack[^1].AddChild(node);
        }

        _nodeStack.Add(node);
    }

    public void EndNode()
    {
        if (_commentDepth > 0)
            return;

        if (_nodeStack.Count == 0)
            return;

        _nodeStack.RemoveAt(_nodeStack.Count - 1);
        return;
    }

    public void Argument(KdlValue value, KdlSourceInfo source)
    {
        if (_commentDepth > 0)
            return;

        if (_nodeStack.Count == 0)
            return;

        KdlNode node = _nodeStack[^1];
        node.Arguments.Add(value);
        return;
    }

    public void Property(string name, KdlValue value, KdlSourceInfo source)
    {
        if (_commentDepth > 0)
            return;

        if (_nodeStack.Count == 0)
            return;

        KdlNode node = _nodeStack[^1];
        node.Properties[name] = value;
        return;
    }
}
