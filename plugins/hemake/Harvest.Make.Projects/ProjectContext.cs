// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class ProjectContext
{
    public EPlatformArch Arch { get; set; } = EPlatformArch.X86_64;
    public string Configuration { get; set; } = "";
    public EPlatformSystem Host { get; set; } = EPlatformSystem.Windows;
    public EModuleLanguage Language { get; set; } = EModuleLanguage.Cpp;
    public SortedDictionary<string, object?> Options { get; set; } = [];
    public string Platform { get; set; } = "";
    public string ProjectPath { get; set; } = "";
    public EPlatformSystem System { get; set; } = EPlatformSystem.Windows;
    public SortedSet<string> Tags { get; set; } = [];
    public EToolset Toolset { get; set; } = EToolset.MSVC;
}
