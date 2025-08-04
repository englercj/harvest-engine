// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class ProjectContext(IProjectService projectService)
{
    public IProjectService ProjectService => projectService;
    public PluginNode? Plugin { get; set; } = null;
    public ModuleNode? Module { get; set; } = null;
    public ConfigurationNode? Configuration { get; set; } = null;
    public PlatformNode? Platform { get; set; } = null;

    public EPlatformSystem Host { get; set; } = EPlatformSystem.Windows;
    public SortedDictionary<string, object?> Options { get; set; } = [];
    public HashSet<string> Tags { get; set; } = [];

    public bool IsWindows => (Platform?.System ?? EPlatformSystem.Windows) == EPlatformSystem.Windows;

    public ProjectContext Clone()
    {
        return (ProjectContext)MemberwiseClone();
    }
}
