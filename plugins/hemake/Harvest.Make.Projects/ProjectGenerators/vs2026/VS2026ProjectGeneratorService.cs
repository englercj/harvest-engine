// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.Logging;
using static Harvest.Make.Projects.ModuleGroupTree;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

[Service<IProjectGeneratorService>(Key = ProjectGeneratorNames.VS2026)]
public class VS2026ProjectGeneratorService(
    IProjectService projectService,
    ILogger<VS2026ProjectGeneratorService> logger,
    ILoggerFactory loggerFactory)
    : IProjectGeneratorService
{
    private readonly IProjectService _projectService = projectService;
    private readonly ILogger _logger = logger;
    private readonly ILoggerFactory _loggerFactory = loggerFactory;
    private readonly ModuleGroupTree _groupTree = new();

    public async Task GenerateProjectFilesAsync()
    {
        BuildModuleTree();

        ProjectNode project = _projectService.GetGlobalNode<ProjectNode>();
        Directory.CreateDirectory(project.BuildDir);

        SlnxGenerator slnGenerator = new(_projectService, _loggerFactory.CreateLogger<SlnxGenerator>());
        await slnGenerator.GenerateAsync(_groupTree);

        List<Task> projectGenerationTasks = [];

        // TODO: Can probably multi-thread the generation of the project files.
        // Reads from project service should be thread-safe and I don't think the
        // order we do this in matters.
        foreach (Entry entry in _groupTree.Entries)
        {
            if (entry is not ModuleEntry moduleEntry)
            {
                continue;
            }

            switch (moduleEntry.Language)
            {
                case EModuleLanguage.C:
                case EModuleLanguage.Cpp:
                {
                    VcxprojGenerator generator = new(_projectService, _loggerFactory.CreateLogger<VcxprojGenerator>());
                    Task task = generator.GenerateAsync(entry.Name);
                    projectGenerationTasks.Add(task);
                    break;
                }
                case EModuleLanguage.CSharp:
                {
                    throw new NotImplementedException("C# project generation has not yet been implemented");
                    //CsprojGenerator generator = new(_projectService,);
                    //Task task = generator.GenerateAsync(entry.Module);
                    //projectGenerationTasks.Add(task);
                    //break;
                }
                default:
                {
                    throw new NotImplementedException($"Unsupported language: {moduleEntry.Language}");
                }
            }
        }

        await Task.WhenAll(projectGenerationTasks);
    }

    private void BuildModuleTree()
    {
        ProjectNode project = _projectService.GetGlobalNode<ProjectNode>();
        Entry? startupEntry = null;

        _groupTree.Clear();
        foreach ((_, ResolvedProjectTree projectTree) in _projectService.ResolvedProjectTrees)
        {
            if (!VisualStudioUtils.IsSupportedPlatform(projectTree.ProjectContext.Platform))
            {
                continue;
            }

            foreach (ModuleNode module in projectTree.IndexedNodes.GetAllNodes<ModuleNode>())
            {
                if (module.Kind != EModuleKind.Content)
                {
                    if (_groupTree.TryAdd(module))
                    {
                        if (module.ModuleName == project.StartupModule)
                        {
                            startupEntry = _groupTree.ModuleEntriesByName[module.ModuleName];
                        }
                    }
                }
            }
        }
        _groupTree.Sort();

        // Move the startup module to the front of the list. This will make VS treat it like a
        // startup project. We do this for each group in the tree, so that the startup module
        // is at the top of the tree and VS will not overwrite our list if the solution is changed.
        while (startupEntry is not null && startupEntry.Parent is not null)
        {
            Entry parent = startupEntry.Parent;
            parent.Children.Remove(startupEntry);
            parent.Children.Insert(0, startupEntry);
            startupEntry = parent;
        }
    }
}
