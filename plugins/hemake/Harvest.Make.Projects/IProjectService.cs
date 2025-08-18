// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;
using System.CommandLine;
using System.CommandLine.Invocation;

namespace Harvest.Make.Projects;

public class ProjectOption(OptionNode node, Option option)
{
    public OptionNode Node => node;
    public Option Option => option;
}

public class ProjectBuildId(string configurationName, string platformName) : IEquatable<ProjectBuildId>
{
    public string ConfigurationName => configurationName;
    public string PlatformName => platformName;

    public override int GetHashCode() => HashCode.Combine(ConfigurationName, PlatformName);
    public override bool Equals(object? obj) => Equals(obj as ProjectBuildId);

    public bool Equals(ProjectBuildId? other)
    {
        if (other is null)
        {
            return false;
        }

        if (ReferenceEquals(this, other))
        {
            return true;
        }

        return ConfigurationName == other.ConfigurationName && PlatformName == other.PlatformName;
    }
}

public class ModuleDependency(DependenciesEntryNode entry, ModuleNode? resolvedModule)
{
    public string DependencyName => entry.DependencyName;
    public EDependencyKind Kind => entry.Kind;
    public bool IsExternal { get; set; } = entry.IsExternal;
    public bool IsWholeArchive { get; set; } = entry.IsWholeArchive;

    public ModuleNode? Module => resolvedModule;
}

public delegate string? CustomStringTokenResolver(ProjectContext projectContext);
public delegate string? CustomStringTokenTransformer(string input);

public interface IProjectService
{
    public string ProjectPath { get; }
    public KdlDocument ProjectDocument { get; }
    public IReadOnlyList<ProjectOption> ProjectOptions { get; }
    public IReadOnlyDictionary<string, object?> ProjectOptionValues { get; }

    public IReadOnlyDictionary<(string, string), CustomStringTokenResolver> TokenResolvers { get; }
    public IReadOnlyDictionary<string, CustomStringTokenTransformer> TokenTransformers { get; }

    public void RegisterNode<T>(bool overwrite = false) where T : class, INode;
    public void RegisterNodeGenerator<T>(bool overwrite = false) where T : class, INodeGenerator;
    public void RegisterTokenResolver(string contextName, string propertyName, CustomStringTokenResolver resolver, bool overwrite = false);
    public void RegisterTokenTransformer(string name, CustomStringTokenTransformer transformer, bool overwrite = false);

    public void LoadProject(string projectPath);
    public void ParseProject(InvocationContext invocationContext);

    public INodeTraits GetNodeTraits(KdlNode node);

    public T CreateSemanticNode<T>(KdlNode node) where T : class, INode;
    public INode CreateSemanticNode(KdlNode node);

    public INodeGenerator CreateGeneratorForNode(KdlNode node);
}
