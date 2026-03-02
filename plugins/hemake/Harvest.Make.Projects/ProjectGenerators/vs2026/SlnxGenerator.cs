// Copyright Chad Engler

using Harvest.Common.Extensions;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.Logging;
using System.Text;
using System.Xml;
using static Harvest.Make.Projects.ModuleGroupTree;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

internal class SlnxGenerator(IProjectService projectService, ILogger<SlnxGenerator> logger)
{
    public const string SolutionExtension = ".slnx";

    private readonly IProjectService _projectService = projectService;
    private readonly ILogger _logger = logger;

    private readonly HashSet<string> _configNames = [];
    private readonly HashSet<string> _platformNames = [];
    private readonly Dictionary<string, string> _platformArchs = [];
    private string _solutionDir = "";

    public async Task GenerateAsync(ModuleGroupTree groupTree)
    {
        ProjectNode project = _projectService.GetGlobalNode<ProjectNode>();
        string outputPath = Path.Join(project.BuildDir, $"{project.ProjectName}{SolutionExtension}");
        _solutionDir = project.BuildDir;
        string projectsDirRelative = Path.GetRelativePath(project.BuildDir, project.ProjectsDir);
        if (projectsDirRelative == ".")
        {
            projectsDirRelative = "";
        }

        _logger.LogDebug("Generating solution file: {SolutionPath}", outputPath);

        foreach ((ResolvedProjectTree projectTree, string archName) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            _configNames.Add(projectTree.ProjectContext.Configuration.ConfigName);
            _platformNames.Add(projectTree.ProjectContext.Platform.PlatformName);
            _platformArchs[projectTree.ProjectContext.Platform.PlatformName] = archName;
        }

        await using MemoryStream newSlnxStream = new(4096);
        await using XmlWriter writer = XmlWriter.Create(newSlnxStream, new XmlWriterSettings
        {
            Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false),
            Async = true,
            Indent = true,
            IndentChars = "  ",
            NewLineChars = Environment.NewLine,
            OmitXmlDeclaration = true,
        });

        writer.WriteStartElement("Solution");
        WriteConfigurations(writer);
        WriteProjects(writer, groupTree, projectsDirRelative);
        writer.WriteEndElement();

        await writer.FlushAsync();

        bool fileChanged = await newSlnxStream.CopyToFileIfChangedAsync(outputPath);
        if (!fileChanged)
        {
            _logger.LogDebug("Solution file is already up to date, skipping write.");
        }
    }

    private void WriteConfigurations(XmlWriter writer)
    {
        writer.WriteStartElement("Configurations");

        foreach (string configName in _configNames)
        {
            writer.WriteStartElement("BuildType");
            writer.WriteAttributeString("Name", configName);
            writer.WriteEndElement();
        }

        foreach (string platformName in _platformNames)
        {
            writer.WriteStartElement("Platform");
            writer.WriteAttributeString("Name", platformName);
            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    private void WriteProjects(XmlWriter writer, ModuleGroupTree groupTree, string projectsDir)
    {
        foreach (Entry entry in groupTree.Root.Children)
        {
            if (entry is ModuleEntry moduleEntry)
            {
                WriteProjectEntry(writer, moduleEntry, projectsDir);
            }
            else
            {
                WriteFolderEntry(writer, entry, projectsDir);
            }
        }
    }

    private void WriteProjectEntry(XmlWriter writer, ModuleEntry entry, string projectsDir)
    {
        string projectPath = GetModuleProjectPath(entry.Name, projectsDir);

        writer.WriteStartElement("Project");
        writer.WriteAttributeString("Path", projectPath);

        foreach ((ResolvedProjectTree project, _) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            if (project.IndexedNodes.TryGetNode(entry.Name, out ModuleNode? module))
            {
                List<ModuleDependency> dependencies = project.GetModuleDependencies(module, ENodeDependencyInheritance.None);
                foreach (ModuleDependency dependency in dependencies)
                {
                    if (dependency.Kind != EDependencyKind.Default
                        && dependency.Kind != EDependencyKind.Link
                        && dependency.Kind != EDependencyKind.Order)
                    {
                        continue;
                    }

                    if (!project.IndexedNodes.TryGetNode(dependency.DependencyName, out ModuleNode? dependencyModule))
                    {
                        continue;
                    }

                    // When we have a Default dependency we want to yield a build order dependency
                    // if the module is a binary and it depends on a module we cannot link, but
                    // needs to run first.
                    if (dependency.Kind == EDependencyKind.Default)
                    {
                        if (!module.IsBinary
                            || dependencyModule.Kind == EModuleKind.LibStatic
                            || dependencyModule.Kind == EModuleKind.LibHeader
                            || dependencyModule.Kind == EModuleKind.Content)
                        {
                            continue;
                        }
                    }

                    writer.WriteStartElement("BuildDependency");
                    writer.WriteAttributeString("Project", GetModuleProjectPath(dependencyModule.ModuleName, projectsDir));
                    writer.WriteEndElement();
                }
            }
        }

        foreach ((ResolvedProjectTree project, _) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            ConfigurationNode config = project.ProjectContext.Configuration;
            PlatformNode platform = project.ProjectContext.Platform;

            writer.WriteStartElement("BuildType");
            writer.WriteAttributeString("Solution", $"{config.ConfigName}|{platform.PlatformName}");
            writer.WriteAttributeString("Project", $"{config.ConfigName} {platform.PlatformName}");
            writer.WriteEndElement();
        }

        foreach ((string platformName, string archName) in _platformArchs)
        {
            writer.WriteStartElement("Platform");
            writer.WriteAttributeString("Solution", $"*|{platformName}");
            writer.WriteAttributeString("Project", archName);
            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    private string GetModuleProjectPath(string moduleName, string projectsDir)
    {
        foreach ((ResolvedProjectTree projectTree, _) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            if (!projectTree.IndexedNodes.TryGetNode(moduleName, out ModuleNode? module))
            {
                continue;
            }

            if (!string.IsNullOrEmpty(module.ProjectFile))
            {
                string projectFile = module.ProjectFile;
                if (!Path.IsPathRooted(projectFile))
                {
                    string moduleDir = Path.GetDirectoryName(module.Node.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();
                    projectFile = Path.GetFullPath(projectFile, moduleDir);
                }

                return Path.GetRelativePath(_solutionDir, projectFile).Replace('\\', '/');
            }

            break;
        }

        string generatedPath = string.IsNullOrEmpty(projectsDir)
            ? $"{moduleName}{VcxprojGenerator.ProjectExtension}"
            : $"{projectsDir}/{moduleName}{VcxprojGenerator.ProjectExtension}";
        return generatedPath.Replace('\\', '/');
    }

    private void WriteFolderEntry(XmlWriter writer, Entry entry, string projectsDir)
    {
        writer.WriteStartElement("Folder");
        writer.WriteAttributeString("Name", $"/{entry.FullPath}/");

        foreach (Entry child in entry.Children)
        {
            if (child is ModuleEntry childModule)
            {
                WriteProjectEntry(writer, childModule, projectsDir);
            }
        }

        writer.WriteEndElement();

        foreach (Entry child in entry.Children)
        {
            if (child is not ModuleEntry)
            {
                WriteFolderEntry(writer, child, projectsDir);
            }
        }
    }
}
