// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;
using System.CommandLine;

namespace Harvest.Make.Projects.Services;

public class ProjectOption(OptionNode node, Option option)
{
    public OptionNode Node => node;
    public Option Option => option;
}

public class ProjectBuildId(string configurationName, string platformName) : IEquatable<ProjectBuildId>, IComparable<ProjectBuildId>
{
    public string ConfigurationName => configurationName;
    public string PlatformName => platformName;

    public override int GetHashCode() => HashCode.Combine(ConfigurationName, PlatformName);
    public override bool Equals(object? obj) => Equals(obj as ProjectBuildId);

    public int CompareTo(ProjectBuildId? other)
    {
        if (other is null)
        {
            return 1;
        }

        int configResult = StringComparer.Ordinal.Compare(ConfigurationName, other.ConfigurationName);
        if (configResult != 0)
        {
            return configResult;
        }

        return StringComparer.Ordinal.Compare(PlatformName, other.PlatformName);
    }

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

public delegate string? CustomStringTokenResolver(ProjectContext projectContext);
public delegate string? CustomStringTokenTransformer(string input);

public interface IProjectService
{
    public string ProjectPath { get; }
    public KdlDocument ProjectDocument { get; }
    public IReadOnlyList<ProjectOption> ProjectOptions { get; }
    public IReadOnlyDictionary<string, object?> ProjectOptionValues { get; }

    public PathGlobCache PathGlobs { get; }
    public IReadOnlyDictionary<(string, string), CustomStringTokenResolver> TokenResolvers { get; }
    public IReadOnlyDictionary<string, CustomStringTokenTransformer> TokenTransformers { get; }
    public IReadOnlyDictionary<ProjectBuildId, ResolvedProjectTree> ResolvedProjectTrees { get; }

    public void RegisterNode<T>(bool overwrite = false) where T : class, INode;
    public void RegisterNodeGenerator<T>(bool overwrite = false) where T : class, INodeGenerator;
    public void RegisterTokenResolver(string contextName, string propertyName, CustomStringTokenResolver resolver, bool overwrite = false);
    public void RegisterTokenTransformer(string name, CustomStringTokenTransformer transformer, bool overwrite = false);

    public void LoadProject(string projectPath);
    public void ParseProject(ParseResult parseResult);

    public T GetGlobalNode<T>() where T : class, INode;
    public INodeTraits GetNodeTraits(KdlNode node);
    public INodeGenerator CreateGeneratorForNode(KdlNode node, NodeResolver resolver);
}
