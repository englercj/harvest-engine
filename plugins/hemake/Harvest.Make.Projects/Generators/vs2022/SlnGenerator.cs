// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects.Generators.vs2022;

internal class SlnGenerator(IProjectService projectService, ProjectGeneratorHelper helper)
{
    private class DescriptorEntry(string name, ConfigurationNode config, PlatformNode platform, string archName)
    {
        public string Name => name;
        public ConfigurationNode Config => config;
        public PlatformNode Platform => platform;
        public string ArchName => archName;
    }

    private readonly IProjectService _projectService = projectService;
    private readonly ProjectGeneratorHelper _helper = helper;
    private readonly IndentedStringBuilder _writer = new();

    public void Generate(ModuleGroupTree groupTree, string formatVersion, string vsVersion)
    {
        _writer.Clear();
        _writer.AppendLine($"Microsoft Visual Studio Solution File, Format Version {formatVersion}");
        _writer.AppendLine($"# Visual Studio Version {vsVersion}");

        WriteProjects(groupTree);

        _writer.AppendLine("Global");
        _writer.IncreaseIndent();
        WriteConfigurationPlatforms(groupTree);
        WriteNestedProjects(groupTree);
        WriteProperties();
        _writer.DecreaseIndent();
        _writer.AppendLine("EndGlobal");

        string slnPath = Path.Join(_helper.BuildOutput.BasePath, $"{_helper.Project.ProjectName}.sln");
        File.WriteAllText(slnPath, _writer.ToString());
    }

    private IEnumerable<ModuleGroupTree.Entry> GetOrderDependencies(ProjectContext projectContext, ModuleNode module, ModuleGroupTree groupTree)
    {
        // TODO: do this ForEachConfig to catch all possible dependencies?

        foreach (DependenciesNode dependency in _projectService.GetNodes<DependenciesNode>(projectContext, module, false))
        {
            foreach (DependenciesEntryNode dependencyEntry in dependency.Entries)
            {
                if (dependencyEntry.Kind != EDependencyKind.Default && dependencyEntry.Kind != EDependencyKind.Order)
                {
                    continue;
                }

                if (groupTree.TryGetEntry(dependencyEntry.DependencyName, out ModuleGroupTree.Entry? entry))
                {
                    if (entry.Module is null)
                    {
                        continue;
                    }

                    // Order dependencies are always included
                    if (dependencyEntry.Kind == EDependencyKind.Order)
                    {
                        yield return entry;
                    }
                    // When we have a Default dependency we want to yield a build order dependency
                    // if the module is a binary and it depends on a module we cannot link, but
                    // needs to run first.
                    else if (module.IsBinary
                        && entry.Module.Kind != EModuleKind.LibStatic
                        && entry.Module.Kind != EModuleKind.LibHeader
                        && entry.Module.Kind != EModuleKind.Content)
                    {
                        yield return entry;
                    }
                }
            }
        }
    }

    private void WriteProjects(ModuleGroupTree groupTree)
    {
        groupTree.Traverse((entry) =>
        {
            if (entry.Module is not null)
            {
                // TODO: Translate path to use VS tokens.
                string projFileExt = VisualStudioUtils.LanguageProjectExtensions[entry.Module.Language];
                string projFileName = $"{entry.Module.ModuleName}{projFileExt}";
                string projPath = Path.Join(_helper.BuildOutput.ProjectDir, projFileName);
                string relPath = VisualStudioUtils.TranslatePath(_helper.BuildOutput.BasePath, projPath);
                string toolId = VisualStudioUtils.LanguageToolIds[entry.Module.Language];
                _writer.AppendLine($"Project(\"{{{toolId}}}\") = \"{entry.Name}\", \"{relPath}\", \"{entry.ID}\"");
                _writer.IncreaseIndent();

                _writer.AppendLine("ProjectSection(ProjectDependencies) = postProject");
                _writer.IncreaseIndent();
                foreach (ModuleGroupTree.Entry dep in GetOrderDependencies(_helper.BaseContext, entry.Module, groupTree))
                {
                    _writer.AppendLine($"{dep.ID} = {dep.ID}");
                }
                _writer.DecreaseIndent();
                _writer.AppendLine("EndProjectSection");

                _writer.DecreaseIndent();
                _writer.AppendLine("EndProject");
            }
            else
            {
                _writer.AppendLine($"Project(\"{{2150E333-8FDC-42A3-9474-1A3956D46DE8}}\") = \"{entry.Name}\", \"{entry.Name}\", \"{entry.ID}\"");
                _writer.AppendLine("EndProject");
            }
        });
    }

    private void WriteConfigurationPlatforms(ModuleGroupTree groupTree)
    {
        List<DescriptorEntry> descriptors = [];
        bool hasAnyDefaultDescriptors = false;

        VisualStudioUtils.ForEachConfig(_helper, (config, platform, archName) =>
        {
            string descriptor = $"{config.ConfigName}|{platform.PlatformName}";
            descriptors.Add(new DescriptorEntry(descriptor, config, platform, archName));
            hasAnyDefaultDescriptors |= platform.IsDefault;
        });

        descriptors.Sort((a, b) => string.Compare(a.Name, b.Name, StringComparison.InvariantCultureIgnoreCase));

        // Write default descriptors first
        if (hasAnyDefaultDescriptors)
        {
            _writer.AppendLine("GlobalSection(SolutionConfigurationPlatforms) = preSolution");
            _writer.IncreaseIndent();
            foreach (DescriptorEntry desc in descriptors)
            {
                if (desc.Platform.IsDefault)
                {
                    _writer.AppendLine($"{desc.Name} = {desc.Name}");
                }
            }
            _writer.DecreaseIndent();
            _writer.AppendLine("EndGlobalSection");
        }

        // Next write non-default descriptors
        {
            _writer.AppendLine("GlobalSection(SolutionConfigurationPlatforms) = preSolution");
            _writer.IncreaseIndent();
            foreach (DescriptorEntry desc in descriptors)
            {
                if (!desc.Platform.IsDefault)
                {
                    _writer.AppendLine($"{desc.Name} = {desc.Name}");
                }
            }
            _writer.DecreaseIndent();
            _writer.AppendLine("EndGlobalSection");
        }

        // Finally, write mappings of project configurations to solution configurations
        _writer.AppendLine("GlobalSection(ProjectConfigurationPlatforms) = postSolution");
        _writer.IncreaseIndent();
        groupTree.Traverse((entry) =>
        {
            if (entry.Module is null)
            {
                return;
            }

            foreach (DescriptorEntry desc in descriptors)
            {
                string configName = $"{desc.Config.ConfigName} {desc.Platform.PlatformName}";
                _writer.AppendLine($"{{{entry.ID}}}.{desc.Name}.ActiveCfg = {configName}|{desc.ArchName}");

                if (entry.Module.Kind != EModuleKind.Content)
                {
                    _writer.AppendLine($"{{{entry.ID}}}.{desc.Name}.Build.0 = {configName}|{desc.ArchName}");
                }
            }
        });
        _writer.DecreaseIndent();
        _writer.AppendLine("EndGlobalSection");
    }

    private void WriteNestedProjects(ModuleGroupTree groupTree)
    {
        if (!groupTree.HasBranches)
        {
            return;
        }

        _writer.AppendLine("GlobalSection(NestedProjects) = preSolution");
        _writer.IncreaseIndent();
        groupTree.Traverse((entry) =>
        {
            if (entry.Parent is not null)
            {
                _writer.AppendLine($"{{{entry.ID}}} = {{{entry.Parent.ID}}}");
            }
        });
        _writer.DecreaseIndent();
        _writer.AppendLine("EndGlobalSection");
    }

    private void WriteProperties()
    {
        _writer.AppendLine("GlobalSection(SolutionProperties) = preSolution");
        _writer.IncreaseIndent();
        _writer.AppendLine("HideSolutionNode = FALSE");
        _writer.DecreaseIndent();
        _writer.AppendLine("EndGlobalSection");
    }
}
