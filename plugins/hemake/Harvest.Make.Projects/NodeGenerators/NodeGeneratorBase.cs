// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects.NodeGenerators;

public abstract class NodeGeneratorBaseTraits : INodeGeneratorTraits
{
    public abstract string Name { get; }
}

public abstract class NodeGeneratorBase<TTraits>(ProjectContext context) : INodeGenerator
    where TTraits : NodeGeneratorBaseTraits, new()
{
    private static readonly TTraits _generatorTraits = new();

    public static INodeGeneratorTraits GeneratorTraits => _generatorTraits;

    public INodeGeneratorTraits Traits => _generatorTraits;

    protected readonly ProjectContext _context = context;

    public abstract void GenerateNodes(KdlNode generatorNode, INode scope);
}
