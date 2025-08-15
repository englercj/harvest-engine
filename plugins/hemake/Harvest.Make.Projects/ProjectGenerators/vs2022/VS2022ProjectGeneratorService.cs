// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.Projects.Nodes;
using System.CommandLine.Invocation;

namespace Harvest.Make.Projects.ProjectGenerators.vs2022;

[Service<IProjectGeneratorService>(Key = ProjectGeneratorNames.VS2022)]
public class VS2022ProjectGeneratorService(IProjectService projectService) : IProjectGeneratorService
{
    private readonly IProjectService _projectService = projectService;
    private readonly ModuleGroupTree _groupTree = new();

    public void GenerateProjectFiles(InvocationContext context)
    {
        ProjectGeneratorHelper helper = new(_projectService, context);

        BuildModuleTree(helper.BaseContext);

        SlnGenerator slnGenerator = new(_projectService, helper);
        slnGenerator.Generate(_groupTree, "12.00", "17");

        // TODO: Can probably multi-thread the generation of the project files.
        // Reads from project service should be thread-safe and I don't think the
        // order we do this in matters.
        _groupTree.Traverse((entry) =>
        {
            if (entry.Module is null)
            {
                return;
            }

            switch (entry.Module.Language)
            {
                case EModuleLanguage.C:
                case EModuleLanguage.Cpp:
                {
                    VcxprojGenerator generator = new(_projectService, helper);
                    generator.Generate(context, entry.Module);
                    break;
                }
                case EModuleLanguage.CSharp:
                {
                    throw new NotImplementedException("C# project generation has not yet been implemented");
                    //CsprojGenerator generator = new(_projectService, helper);
                    //generator.Generate(context, entry.Module);
                    //break;
                }
                default:
                {
                    throw new NotImplementedException($"Unsupported language: {entry.Module.Language}");
                }
            }
        });
    }

    private void BuildModuleTree(ProjectContext projectContext)
    {
        ProjectNode projectNode = _projectService.GetMergedNode<ProjectNode>(projectContext);
        List<ModuleNode> modules = _projectService.GetNodes<ModuleNode>(projectContext);

        // Create and sort the tree of modules.
        _groupTree.Clear();
        foreach (ModuleNode module in modules)
        {
            if (module.Kind != EModuleKind.Content)
            {
                _groupTree.Add(module);
            }
        }
        _groupTree.Sort();

        // Check if we have a startup project that required reordering the tree nodes.
        if (projectNode.StartupModule is null)
        {
            return;
        }

        // Find the entry that is marked as the startup module.
        ModuleGroupTree.Entry? startupEntry = null;
        _groupTree.Traverse((entry) =>
        {
            if (entry.Module is not null && entry.Module.ModuleName == projectNode.StartupModule)
            {
                startupEntry = entry;
            }
        });

        // Move the startup module to the front of the list. This will make VS treat it like a
        // startup project. We do this for each group in the tree, so that the startup module
        // is at the top of the tree and VS will not overwrite our list if the solution is changed.
        while (startupEntry is not null && startupEntry.Parent is not null)
        {
            ModuleGroupTree.Entry parent = startupEntry.Parent;
            parent.Children.Remove(startupEntry);
            parent.Children.Insert(0, startupEntry);
            startupEntry = parent;
        }
    }
}
