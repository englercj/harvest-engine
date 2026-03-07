// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Services;

namespace Harvest.Make.Projects.NodeGenerators;

public interface INodeGeneratorTraits
{
    public string Name { get; }

    public INodeGenerator CreateGenerator(IProjectService projectService, NodeResolver resolver);

    public static T CreateGenerator<T>(IProjectService projectService, NodeResolver resolver) where T : class, INodeGenerator =>
        (T)T.GeneratorTraits.CreateGenerator(projectService, resolver);
}

public interface INodeGenerator
{
    public static virtual INodeGeneratorTraits GeneratorTraits => throw new NotImplementedException();

    public INodeGeneratorTraits Traits { get; }

    /// <summary>
    /// Perform the node generation logic for this generator.
    /// </summary>
    /// <param name="target">The target node to add generated nodes to.</param>
    /// <param name="generatorNode">The source KDL node for the generator.</param>
    public void GenerateNodes(KdlNode target, KdlNode generatorNode);
}
