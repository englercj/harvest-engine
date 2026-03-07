using Harvest.Common.Extensions;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.Logging;
using System.Diagnostics.CodeAnalysis;
using System.Text;
using System.Xml;
using System.Xml.Linq;
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
    private readonly Dictionary<string, ExternalProjectConfig> _externalProjectConfigs = [];
    private string _solutionDir = "";

    private sealed record ExternalProjectConfig(IReadOnlyList<string> BuildTypes, IReadOnlyList<string> Platforms);

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
        HashSet<string> buildDependencyPaths = [];

        string? externalProjectFilePath = null;
        ExternalProjectConfig? externalProjectConfig = null;
        if (TryGetModuleProjectFilePath(entry.Name, out string? entryProjectFilePath)
            && string.Equals(Path.GetExtension(entryProjectFilePath), ".csproj", StringComparison.OrdinalIgnoreCase))
        {
            externalProjectFilePath = entryProjectFilePath;
            externalProjectConfig = GetExternalProjectConfig(entryProjectFilePath);
        }

        writer.WriteStartElement("Project");
        writer.WriteAttributeString("Path", projectPath);

        foreach ((ResolvedProjectTree project, _) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            if (project.IndexedNodes.TryGetNode(entry.Name, out ModuleNode? module))
            {
                List<ModuleDependency> dependencies = project.GetModuleDependencies(module, ENodeDependencyInheritance.All);
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

                    string dependencyProjectPath = GetModuleProjectPath(dependencyModule.ModuleName, projectsDir);
                    if (dependencyProjectPath == projectPath || !buildDependencyPaths.Add(dependencyProjectPath))
                    {
                        continue;
                    }

                    writer.WriteStartElement("BuildDependency");
                    writer.WriteAttributeString("Project", dependencyProjectPath);
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
            writer.WriteAttributeString("Project", externalProjectConfig is null
                ? $"{config.ConfigName} {platform.PlatformName}"
                : MapExternalProjectBuildType(externalProjectFilePath!, config.ConfigName, externalProjectConfig));
            writer.WriteEndElement();
        }

        foreach ((string platformName, string archName) in _platformArchs)
        {
            writer.WriteStartElement("Platform");
            writer.WriteAttributeString("Solution", $"*|{platformName}");
            writer.WriteAttributeString("Project", externalProjectConfig is null
                ? archName
                : MapExternalProjectPlatform(externalProjectFilePath!, platformName, externalProjectConfig));
            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    private bool TryGetModuleProjectFilePath(string moduleName, [NotNullWhen(true)] out string? projectFilePath)
    {
        foreach ((ResolvedProjectTree projectTree, _) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            if (!projectTree.IndexedNodes.TryGetNode(moduleName, out ModuleNode? module))
            {
                continue;
            }

            if (!string.IsNullOrEmpty(module.ProjectFile))
            {
                projectFilePath = GetResolvedProjectFilePath(module);
                return true;
            }

            break;
        }

        projectFilePath = null;
        return false;
    }

    private string GetModuleProjectPath(string moduleName, string projectsDir)
    {
        if (TryGetModuleProjectFilePath(moduleName, out string? projectFilePath))
        {
            return Path.GetRelativePath(_solutionDir, projectFilePath!).Replace('\\', '/');
        }

        string generatedPath = string.IsNullOrEmpty(projectsDir)
            ? $"{moduleName}{VcxprojGenerator.ProjectExtension}"
            : $"{projectsDir}/{moduleName}{VcxprojGenerator.ProjectExtension}";
        return generatedPath.Replace('\\', '/');
    }

    private static string GetResolvedProjectFilePath(ModuleNode module)
    {
        string? projectFile = module.ProjectFile;

        if (string.IsNullOrEmpty(projectFile))
        {
            throw new InvalidOperationException($"Module '{module.ModuleName}' does not define a project_file.");
        }

        if (!Path.IsPathRooted(projectFile))
        {
            string moduleDir = Path.GetDirectoryName(module.Node.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();
            projectFile = Path.GetFullPath(projectFile, moduleDir);
        }

        return projectFile;
    }

    private ExternalProjectConfig GetExternalProjectConfig(string projectFilePath)
    {
        if (_externalProjectConfigs.TryGetValue(projectFilePath, out ExternalProjectConfig? config))
        {
            return config;
        }

        config = LoadExternalProjectConfig(projectFilePath);
        _externalProjectConfigs.Add(projectFilePath, config);
        return config;
    }

    private static ExternalProjectConfig LoadExternalProjectConfig(string projectFilePath)
    {
        List<string> buildTypes = ["Debug", "Release"];
        List<string> platforms = ["Any CPU"];

        if (!File.Exists(projectFilePath))
        {
            return new ExternalProjectConfig(buildTypes, platforms);
        }

        XDocument document = XDocument.Load(projectFilePath, LoadOptions.PreserveWhitespace);

        foreach (XElement element in document.Descendants())
        {
            if (string.Equals(element.Name.LocalName, "Configurations", StringComparison.Ordinal))
            {
                buildTypes = SplitProjectValues(element.Value, ["Debug", "Release"], normalizePlatforms: false);
            }
            else if (string.Equals(element.Name.LocalName, "Platforms", StringComparison.Ordinal))
            {
                platforms = SplitProjectValues(element.Value, ["Any CPU"], normalizePlatforms: true);
            }
        }

        return new ExternalProjectConfig(buildTypes, platforms);
    }

    private static List<string> SplitProjectValues(string? values, IReadOnlyList<string> fallbackValues, bool normalizePlatforms)
    {
        List<string> result = [];

        if (!string.IsNullOrWhiteSpace(values))
        {
            foreach (string rawValue in values.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
            {
                string value = normalizePlatforms ? NormalizePlatformName(rawValue) : rawValue;
                if (!result.Contains(value))
                {
                    result.Add(value);
                }
            }
        }

        if (result.Count == 0)
        {
            result.AddRange(fallbackValues);
        }

        return result;
    }

    private void LogProjectMappingWarning(string projectFilePath, string mappingType, string sourceValue, string fallbackValue)
    {
        _logger.LogWarning(
            "Project file '{ProjectFilePath}' does not define {MappingType} '{SourceValue}'. Falling back to '{FallbackValue}'.",
            projectFilePath,
            mappingType,
            sourceValue,
            fallbackValue);
    }

    private string MapExternalProjectBuildType(string projectFilePath, string solutionBuildType, ExternalProjectConfig config)
    {
        if (TryGetMatchingValue(config.BuildTypes, solutionBuildType, out string? exactMatch))
        {
            return exactMatch;
        }

        if (string.Equals(solutionBuildType, "Development", StringComparison.OrdinalIgnoreCase))
        {
            if (TryGetSimilarValue(config.BuildTypes, "Release", out string? releaseMatch))
            {
                return releaseMatch;
            }

            if (TryGetSimilarValue(config.BuildTypes, "Debug", out string? debugMatch))
            {
                return debugMatch;
            }
        }
        else
        {
            if (TryGetSimilarValue(config.BuildTypes, "Debug", out string? debugMatch))
            {
                return debugMatch;
            }

            if (TryGetSimilarValue(config.BuildTypes, "Release", out string? releaseMatch))
            {
                return releaseMatch;
            }
        }

        string fallbackBuildType = config.BuildTypes.Count != 0 ? config.BuildTypes[0] : "Debug";
        LogProjectMappingWarning(projectFilePath, "build type", solutionBuildType, fallbackBuildType);
        return fallbackBuildType;
    }

    private string MapExternalProjectPlatform(string projectFilePath, string solutionPlatform, ExternalProjectConfig config)
    {
        string normalizedPlatform = NormalizePlatformName(solutionPlatform);
        if (TryGetMatchingValue(config.Platforms, normalizedPlatform, out string? exactMatch))
        {
            return exactMatch;
        }

        if (TryGetMatchingValue(config.Platforms, "Any CPU", out string? anyCpuMatch))
        {
            return anyCpuMatch;
        }

        string fallbackPlatform = config.Platforms.Count != 0 ? config.Platforms[0] : "Any CPU";
        LogProjectMappingWarning(projectFilePath, "platform", solutionPlatform, fallbackPlatform);
        return fallbackPlatform;
    }

    private static bool TryGetMatchingValue(IReadOnlyList<string> values, string expectedValue, [NotNullWhen(true)] out string? match)
    {
        foreach (string value in values)
        {
            if (string.Equals(value, expectedValue, StringComparison.OrdinalIgnoreCase))
            {
                match = value;
                return true;
            }
        }

        match = null;
        return false;
    }

    private static bool TryGetSimilarValue(IReadOnlyList<string> values, string expectedValue, [NotNullWhen(true)] out string? match)
    {
        foreach (string value in values)
        {
            if (value.Contains(expectedValue, StringComparison.OrdinalIgnoreCase))
            {
                match = value;
                return true;
            }
        }

        match = null;
        return false;
    }

    private static string NormalizePlatformName(string platformName)
    {
        return string.Equals(platformName, "AnyCPU", StringComparison.OrdinalIgnoreCase) ? "Any CPU" : platformName;
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
