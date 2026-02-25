// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class ProjectContext
{
    public required IProjectService ProjectService { get; init; }
    public required ConfigurationNode Configuration { get; init; }
    public required PlatformNode Platform { get; init; }
    public required BuildOutputNode BuildOutput { get; set; }

    public PluginNode? Plugin { get; set; } = null;
    public ModuleNode? Module { get; set; } = null;

    public EPlatformSystem Host { get; init; } = EPlatformSystem.Windows;
    public IReadOnlyDictionary<string, object?> Options { get; init; } = new SortedDictionary<string, object?>();
    public HashSet<string> Tags { get; init; } = [];
}
