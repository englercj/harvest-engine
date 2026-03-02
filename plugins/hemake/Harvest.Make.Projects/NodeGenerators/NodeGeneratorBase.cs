// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Services;

namespace Harvest.Make.Projects.NodeGenerators;

public abstract class NodeGeneratorBaseTraits : INodeGeneratorTraits
{
    public abstract string Name { get; }

    public abstract INodeGenerator CreateGenerator(IProjectService projectService, NodeResolver resolver);
}

public abstract class NodeGeneratorBase<TTraits>(IProjectService projectService, NodeResolver resolver) : INodeGenerator
    where TTraits : NodeGeneratorBaseTraits, new()
{
    private static readonly TTraits _generatorTraits = new();

    public static INodeGeneratorTraits GeneratorTraits => _generatorTraits;

    public INodeGeneratorTraits Traits => _generatorTraits;

    protected readonly IProjectService _projectService = projectService;
    protected readonly NodeResolver _resolver = resolver;

    public abstract void GenerateNodes(KdlNode target, KdlNode generatorNode);
}
