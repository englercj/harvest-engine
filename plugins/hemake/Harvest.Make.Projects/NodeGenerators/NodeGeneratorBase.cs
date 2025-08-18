// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.NodeGenerators;

public abstract class NodeGeneratorBaseTraits : INodeGeneratorTraits
{
    public abstract string Name { get; }
}

public abstract class NodeGeneratorBase<TTraits>(IProjectService projectService) : INodeGenerator
    where TTraits : NodeGeneratorBaseTraits, new()
{
    private static readonly TTraits _generatorTraits = new();

    public static INodeGeneratorTraits GeneratorTraits => _generatorTraits;

    public INodeGeneratorTraits Traits => _generatorTraits;

    protected readonly IProjectService _projectService = projectService;

    public abstract void GenerateNodes(KdlNode generatorNode, KdlNode scope);
}
