// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects.NodeGenerators;

public interface INodeGenerator
{
    public static virtual string GeneratorName => throw new NotImplementedException();

    /// <summary>
    /// Perform the node generation logic for this generator.
    /// </summary>
    /// <param name="generatorNode">The source KDL node for the generator.</param>
    /// <param name="scope">The parsed parent node of the generator, where newly generated nodes should be inserted.</param>
    public void GenerateNodes(KdlNode generatorNode, INode scope);
}
