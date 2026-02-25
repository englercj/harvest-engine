// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Xml;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

public interface IVisualStudioFileGroup
{
    public class FileEntry(ResolvedProjectTree projectTree, ModuleNode module, string filePath)
    {
        public ResolvedProjectTree ProjectTree => projectTree;
        public ModuleNode Module => module;
        public string FullPath => filePath;

        public EFileAction Action { get; set; } = EFileAction.Default;
        public EFileBuildRule BuildRule { get; set; } = EFileBuildRule.Default;
        public string BuildRuleName { get; set; } = string.Empty;
        public string VirtualPath { get; set; } = string.Empty;
        public string DependsOnPath { get; set; } = string.Empty;
        public bool IsExcludedFromBuild { get; set; } = false;
    }

    public int Priority { get; }
    public string GroupTag { get; }
    public string VSProjectPath { get; }
    public IReadOnlyList<FileEntry> Files { get; }

    public bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule);
    public bool CanHandleFile(FilesEntryNode entry);

    public void AddFile(ResolvedProjectTree projectTree, FilesEntryNode entry);
    public void AddGeneratedFile(ResolvedProjectTree projectTree, string generatedFilePath, string sourceFilePath, EFileAction action, EFileBuildRule buildRule);

    public void SortFiles();
    public void SetupVirtualPaths();

    public void WriteFiles(XmlWriter writer);
    public void WriteFilters(XmlWriter writer);
    public void WriteExtensionSettings(XmlWriter writer);
    public void WriteExtensionTargets(XmlWriter writer);
}
