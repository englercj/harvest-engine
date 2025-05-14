// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Xml;
using static Harvest.Make.Projects.Generators.vs2022.IVisualStudioFileGroup;

namespace Harvest.Make.Projects.Generators.vs2022;

public abstract class VisualStudioFileGroupBase(ProjectGeneratorHelper helper, string vsProjectPath) : IVisualStudioFileGroup
{
    protected readonly ProjectGeneratorHelper _helper = helper;

    public abstract int Priority { get; }
    public abstract string GroupTag { get; }
    public string VSProjectPath => vsProjectPath;

    private readonly List<FileEntry> _files = [];
    public IReadOnlyList<FileEntry> Files => _files;

    public abstract bool CanHandleFile(FilesEntryNode entry);

    public void AddFile(ProjectContext context, FilesEntryNode entry)
    {
        _files.Add(new FileEntry(entry, context));
    }

    public void SortFiles()
    {
        _files.Sort((a, b) =>
        {
            string relPathA = GetPath(a.Entry.ResolvedFilePath);
            string relPathB = GetPath(b.Entry.ResolvedFilePath);
            return string.Compare(relPathA, relPathB);
        });
    }

    public void WriteFiles(XmlWriter writer)
    {
        if (Files.Count == 0)
        {
            return;
        }

        writer.WriteStartElement("ItemGroup");

        foreach (FileEntry file in Files)
        {
            if (string.IsNullOrEmpty(file.Entry.ResolvedFilePath))
            {
                continue;
            }

            writer.WriteStartElement(GroupTag);
            writer.WriteAttributeString("Include", GetPath(file));

            OnWriteFile(writer, file);
            VisualStudioUtils.ForEachConfig(_helper, (ConfigurationNode configuration, PlatformNode platform, string archName) =>
            {
                OnWriteFileConfig(writer, file, configuration, platform, archName);
            });

            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    public void WriteFilter(XmlWriter writer)
    {
        if (Files.Count == 0)
        {
            return;
        }

        writer.WriteStartElement("ItemGroup");

        foreach (FileEntry file in Files)
        {
            if (string.IsNullOrEmpty(file.Entry.ResolvedFilePath))
            {
                continue;
            }

            writer.WriteStartElement(GroupTag);
            writer.WriteAttributeString("Include", GetPath(file));
            // TODO: grouping of files based on their path relative to the install dir of a module
            if (filePath.parent.path)
            {
                writer.WriteElementString("Filter", filePath.parent.path);
            }
            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    public void WriteExtensionSettings(XmlWriter writer)
    {
        if (Files.Count == 0)
        {
            return;
        }

        OnWriteExtensionSettings(writer);
    }

    public void WriteExtensionTargets(XmlWriter writer)
    {
        if (Files.Count == 0)
        {
            return;
        }

        OnWriteExtensionTargets(writer);
    }

    protected virtual void OnWriteFile(XmlWriter writer, FileEntry file)
    {
        // Nothing by default
    }

    protected virtual void OnWriteFileConfig(XmlWriter writer, FileEntry file, ConfigurationNode configuration, PlatformNode platform, string archName)
    {
        // Nothing by default
    }

    protected virtual void OnWriteExtensionSettings(XmlWriter writer)
    {
        // Nothing by default
    }

    protected virtual void OnWriteExtensionTargets(XmlWriter writer)
    {
        // Nothing by default
    }

    protected string GetPath(string path)
    {
        return VisualStudioUtils.TranslatePath(VSProjectPath, path);
    }

    protected string GetPath(FileEntry file)
    {
        return GetPath(file.Entry.ResolvedFilePath);
    }

    protected static void HandleGeneratedFile(XmlWriter writer, FileEntry file)
    {
        if (file.Entry.IsGenerated)
        {
            string path = path.translate(entry.dependsOn.relpath);
            writer.WriteElementString("AutoGen", "true");
            writer.WriteElementString("DependentUpon", path);
        }
    }

    protected static void HandleExcludedFile(XmlWriter writer, FileEntry file, ConfigurationNode configuration, PlatformNode platform, string archName)
    {
        if (file.Entry.IsExcludedFromBuild)
        {
            string condition = VisualStudioUtils.GetConfigCondition(configuration, platform, archName);
            VisualStudioUtils.WriteElementString(writer, "ExcludedFromBuild", "true", condition);
        }
    }
}
