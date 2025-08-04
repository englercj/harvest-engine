// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects;

public class NodeParseException(KdlNode node, string errorMessage)
    : Exception($"Failed to parse '{node.Name}' node.\n{node.SourceInfo.ToErrorString()}: {errorMessage}")
{
    public KdlNode Node => node;
}
