// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Xml;

namespace Harvest.Make.Projects.Generators.vs2022;

public interface IVisualStudioFileGroup
{
    public struct FileEntry(FilesEntryNode entry, ProjectContext context)
    {
        public readonly FilesEntryNode Entry => entry;
        public readonly ProjectContext Context => context;
    }

    public int Priority { get; }
    public string GroupTag { get; }
    public string VSProjectPath { get; }
    public IReadOnlyList<FileEntry> Files { get; }

    public bool CanHandleFile(FilesEntryNode entry);

    public void AddFile(ProjectContext context, FilesEntryNode entry);
    public void SortFiles();

    public void WriteFiles(XmlWriter writer);
    public void WriteFilter(XmlWriter writer);
    public void WriteExtensionSettings(XmlWriter writer);
    public void WriteExtensionTargets(XmlWriter writer);
}
