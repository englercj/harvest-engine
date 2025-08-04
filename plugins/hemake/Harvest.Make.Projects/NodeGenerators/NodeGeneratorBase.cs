// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects.NodeGenerators;

public abstract class NodeGeneratorBase(ProjectContext context) : INodeGenerator
{
    protected readonly ProjectContext _context = context;

    public abstract void Resolve(KdlNode generatorNode, INode scope);
}
