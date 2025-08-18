// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class ProjectContext
{
    public required IProjectService ProjectService { get; init; }
    //public PluginNode? Plugin { get; init; } = null;
    //public ModuleNode? Module { get; init; } = null;
    public required ConfigurationNode Configuration { get; init; }
    public required PlatformNode Platform { get; init; }

    public EPlatformSystem Host { get; init; } = EPlatformSystem.Windows;
    public IReadOnlyDictionary<string, object?> Options { get; init; } = new SortedDictionary<string, object?>();
    public IReadOnlySet<string> Tags { get; init; } = new HashSet<string>();
}
