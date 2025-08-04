// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects.NodeGenerators;

public interface INodeGenerator
{
    public static virtual string GeneratorName => throw new NotImplementedException();

    public void Resolve(KdlNode generatorNode, INode scope);
}
