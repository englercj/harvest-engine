// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class NodeValidationException(INode node, string errorMessage)
    : Exception($"Validation failed for '{node.Node.Name}' node.\n{node.Node.SourceInfo.ToErrorString()}: {errorMessage}")
{
    public INode Node => node;
}
