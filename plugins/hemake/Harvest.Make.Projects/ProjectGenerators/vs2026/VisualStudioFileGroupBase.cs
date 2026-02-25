// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Xml;
using static Harvest.Make.Projects.ProjectGenerators.vs2026.IVisualStudioFileGroup;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

public abstract class VisualStudioFileGroupBase(IProjectService projectService, string vsProjectPath) : IVisualStudioFileGroup
{
    public abstract int Priority { get; }
    public abstract string GroupTag { get; }
    public string VSProjectPath => vsProjectPath;

    public IProjectService ProjectService => projectService;

    private readonly List<FileEntry> _files = [];
    public IReadOnlyList<FileEntry> Files => _files;

    private readonly List<FileEntry> _generatedFiles = [];
    public IReadOnlyList<FileEntry> GeneratedFiles => _files;

    public abstract bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule);

    public bool CanHandleFile(FilesEntryNode entry)
    {
        return CanHandleFile(entry.FilePath, entry.FileAction, entry.FileBuildRule);
    }

    public void AddFile(ResolvedProjectTree projectTree, FilesEntryNode entry)
    {
        _files.Add(new FileEntry(projectTree, entry.FilePath)
        {
            Action = entry.FileAction,
            BuildRule = entry.FileBuildRule,
            BuildRuleName = entry.BuildRuleName,
            IsExcludedFromBuild = entry.IsExcludedFromBuild,
        });
    }

    public void AddGeneratedFile(ResolvedProjectTree projectTree, string generatedFilePath, string sourceFilePath, EFileAction action, EFileBuildRule buildRule)
    {
        _generatedFiles.Add(new FileEntry(projectTree, generatedFilePath)
        {
            Action = action,
            BuildRule = buildRule,
            BuildRuleName = string.Empty,
            DependsOnPath = sourceFilePath,
            IsExcludedFromBuild = false, // TODO: make this configurable?
        });
    }

    public void SortFiles()
    {
        _files.Sort((a, b) =>
        {
            string relPathA = GetPath(a.FullPath);
            string relPathB = GetPath(b.FullPath);
            return string.Compare(relPathA, relPathB);
        });
    }

    public void SetupVirtualPaths()
    {
        if (Files.Count == 0)
        {
            return;
        }

        string commonDir = GetCommonParentDirectoryName();
        int commonDirLength = commonDir.Length;

        foreach (FileEntry file in _files)
        {
            file.VirtualPath = file.FullPath[commonDirLength..];
        }
    }

    public void WriteFiles(XmlWriter writer)
    {
        if (Files.Count == 0 && GeneratedFiles.Count == 0)
        {
            return;
        }

        writer.WriteStartElement("ItemGroup");

        foreach (FileEntry file in Files)
        {
            writer.WriteStartElement(GroupTag);
            writer.WriteAttributeString("Include", GetPath(file));

            OnWriteFile(writer, file);
            foreach ((ResolvedProjectTree projectTree, string archName) in VisualStudioUtils.EnumerateConfigs(ProjectService))
            {
                OnWriteFileConfig(writer, file, projectTree, archName);
            }

            writer.WriteEndElement();
        }

        foreach (FileEntry file in GeneratedFiles)
        {
            writer.WriteStartElement(GroupTag);
            writer.WriteAttributeString("Include", GetPath(file));

            writer.WriteElementString("AutoGen", "true");

            if (!string.IsNullOrEmpty(file.DependsOnPath))
            {
                writer.WriteElementString("DependentUpon", GetPath(file.DependsOnPath));
            }

            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    public void WriteFilters(XmlWriter writer)
    {
        if (Files.Count == 0 && GeneratedFiles.Count == 0)
        {
            return;
        }

        writer.WriteStartElement("ItemGroup");

        foreach (FileEntry file in Files)
        {
            string fullPath = file.FullPath;
            string relPath = GetPath(fullPath);

            writer.WriteStartElement(GroupTag);
            writer.WriteAttributeString("Include", relPath);
            writer.WriteElementString("Filter", file.VirtualPath);
            writer.WriteEndElement();
        }

        foreach (FileEntry file in GeneratedFiles)
        {
            string fullPath = file.FullPath;
            string relPath = GetPath(fullPath);

            writer.WriteStartElement(GroupTag);
            writer.WriteAttributeString("Include", relPath);
            writer.WriteElementString("Filter", file.VirtualPath);
            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    public void WriteExtensionSettings(XmlWriter writer)
    {
        if (Files.Count == 0 && GeneratedFiles.Count == 0)
        {
            return;
        }

        OnWriteExtensionSettings(writer);
    }

    public void WriteExtensionTargets(XmlWriter writer)
    {
        if (Files.Count == 0 && GeneratedFiles.Count == 0)
        {
            return;
        }

        OnWriteExtensionTargets(writer);
    }

    protected virtual void OnWriteFile(XmlWriter writer, FileEntry file)
    {
        // Nothing by default
    }

    protected virtual void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
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
        return GetPath(file.FullPath);
    }

    protected static void HandleExcludedFile(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        if (file.IsExcludedFromBuild)
        {
            string condition = VisualStudioUtils.GetConfigCondition(projectTree, archName);
            VisualStudioUtils.WriteElementString(writer, "ExcludedFromBuild", "true", condition);
        }
    }

    protected string GetCommonParentDirectoryName()
    {
        int commonStrLen = Files[0].FullPath.Length;

        for (int i = 1; i < Files.Count; i++)
        {
            FileEntry file = Files[i];
            string fullPath = file.FullPath;

            commonStrLen = Math.Min(commonStrLen, fullPath.Length);

            for (int j = 0; j < commonStrLen; j++)
            {
                if (Files[i].FullPath[j] != Files[0].FullPath[j])
                {
                    commonStrLen = j;
                    break;
                }
            }
        }

        string commonStr = Files[0].FullPath[..commonStrLen];

        // If it doesn't end with a path separator, then the last segment is a string prefix
        // of a file or directory name. We remove it to get the common ancestor directory path.
        if (!commonStr.EndsWith(Path.PathSeparator))
        {
            string? dirName = Path.GetDirectoryName(commonStr);
            commonStr = string.IsNullOrEmpty(dirName) ? string.Empty : dirName + Path.PathSeparator;
        }

        return commonStr;
    }
}
