// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using System.Xml;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

public interface IVisualStudioFileGroup
{
    public class FileConfigEntry(ResolvedProjectTree projectTree, ModuleNode module)
    {
        public ResolvedProjectTree ProjectTree => projectTree;
        public ModuleNode Module => module;

        public EFileAction Action { get; set; } = EFileAction.Default;
        public EFileBuildRule BuildRule { get; set; } = EFileBuildRule.Default;
        public string BuildRuleName { get; set; } = "";
        public string DependsOnPath { get; set; } = "";
        public bool IsExcludedFromBuild { get; set; } = false;
    }

    public class FileEntry(string filePath)
    {
        private readonly Dictionary<ProjectBuildId, FileConfigEntry> _configs = [];

        public string FullPath => filePath;
        public string VirtualPath { get; set; } = "";

        public IEnumerable<FileConfigEntry> Configs => _configs.Values;

        public void AddConfig(FileConfigEntry config)
        {
            ProjectBuildId buildId = new(config.ProjectTree.ProjectContext.Configuration.ConfigName, config.ProjectTree.ProjectContext.Platform.PlatformName);
            _configs[buildId] = config;
        }

        public bool TryGetConfig(ResolvedProjectTree projectTree, out FileConfigEntry? config)
        {
            ProjectBuildId buildId = new(projectTree.ProjectContext.Configuration.ConfigName, projectTree.ProjectContext.Platform.PlatformName);
            return _configs.TryGetValue(buildId, out config);
        }
    }

    public int Priority { get; }
    public string GroupTag { get; }
    public string VSProjectPath { get; }
    public IReadOnlyList<FileEntry> Files { get; }
    public IReadOnlyList<FileEntry> GeneratedFiles { get; }

    public bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule);
    public bool CanHandleFile(FilesEntryNode entry);

    public void AddFile(ResolvedProjectTree projectTree, ModuleNode module, FilesEntryNode entry);
    public void AddGeneratedFile(ResolvedProjectTree projectTree, ModuleNode module, string generatedFilePath, string sourceFilePath, EFileAction action, EFileBuildRule buildRule);

    public void SortFiles();
    public void SetupVirtualPaths();
    public void SetupVirtualPaths(string commonDir);

    public void WriteFiles(XmlWriter writer);
    public void WriteFilters(XmlWriter writer);
    public void WriteExtensionSettings(XmlWriter writer);
    public void WriteExtensionTargets(XmlWriter writer);
}
