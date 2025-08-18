// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.NodeGenerators;

public interface INodeGeneratorTraits
{
    public string Name { get; }
}

public interface INodeGenerator
{
    public static virtual INodeGeneratorTraits GeneratorTraits => throw new NotImplementedException();

    public INodeGeneratorTraits Traits { get; }

    /// <summary>
    /// Perform the node generation logic for this generator.
    /// </summary>
    /// <param name="generatorNode">The source KDL node for the generator.</param>
    /// <param name="scope">The scope in which to add generated nodes.</param>
    public void GenerateNodes(KdlNode generatorNode, KdlNode scope);
}
