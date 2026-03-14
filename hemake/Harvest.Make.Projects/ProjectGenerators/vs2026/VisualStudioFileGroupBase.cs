// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using System.Xml;
using static Harvest.Make.Projects.ProjectGenerators.vs2026.IVisualStudioFileGroup;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

internal abstract class VisualStudioFileGroupBase(IProjectService projectService, string vsProjectPath) : IVisualStudioFileGroup
{
    public abstract int Priority { get; }
    public abstract string GroupTag { get; }
    public string VSProjectPath => vsProjectPath;

    public IProjectService ProjectService => projectService;

    private readonly List<FileEntry> _files = [];
    public IReadOnlyList<FileEntry> Files => _files;

    private readonly List<FileEntry> _generatedFiles = [];
    public IReadOnlyList<FileEntry> GeneratedFiles => _generatedFiles;

    public abstract bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule);

    public bool CanHandleFile(FilesEntryNode entry)
    {
        return CanHandleFile(entry.FilePath, entry.FileAction, entry.FileBuildRule);
    }

    public void AddFile(ResolvedProjectTree projectTree, ModuleNode module, FilesEntryNode entry)
    {
        FileEntry file = GetOrAddFile(_files, entry.FilePath);
        file.AddConfig(new FileConfigEntry(projectTree, module)
        {
            Action = entry.FileAction,
            BuildRule = entry.FileBuildRule,
            BuildRuleName = entry.BuildRuleName,
            IsExcludedFromBuild = entry.IsExcludedFromBuild,
        });
    }

    public void AddGeneratedFile(ResolvedProjectTree projectTree, ModuleNode module, string generatedFilePath, string sourceFilePath, EFileAction action, EFileBuildRule buildRule)
    {
        FileEntry file = GetOrAddFile(_generatedFiles, generatedFilePath);
        file.AddConfig(new FileConfigEntry(projectTree, module)
        {
            Action = action,
            BuildRule = buildRule,
            BuildRuleName = "",
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
        _generatedFiles.Sort((a, b) =>
        {
            string relPathA = GetPath(a.FullPath);
            string relPathB = GetPath(b.FullPath);
            return string.Compare(relPathA, relPathB);
        });
    }

    public void SetupVirtualPaths()
    {
        List<FileEntry> allFiles = [.. _files, .. _generatedFiles];
        if (allFiles.Count == 0)
        {
            return;
        }

        string commonDir = GetCommonParentDirectoryName(allFiles);
        SetupVirtualPaths(commonDir);
    }

    public void SetupVirtualPaths(string commonDir)
    {
        List<FileEntry> allFiles = [.. _files, .. _generatedFiles];
        if (allFiles.Count == 0)
        {
            return;
        }

        int commonDirLength = commonDir.Length;

        foreach (FileEntry file in allFiles)
        {
            file.VirtualPath = file.FullPath[commonDirLength..];
        }
    }

    internal static string GetFilterPath(string virtualPath)
    {
        int slashIndex = Math.Max(virtualPath.LastIndexOf('/'), virtualPath.LastIndexOf('\\'));
        return slashIndex >= 0 ? virtualPath[..slashIndex] : string.Empty;
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

            FileConfigEntry? firstConfig = file.Configs.FirstOrDefault();
            if (firstConfig is not null && !string.IsNullOrEmpty(firstConfig.DependsOnPath))
            {
                writer.WriteElementString("DependentUpon", GetPath(firstConfig.DependsOnPath));
            }

            OnWriteFile(writer, file);
            foreach ((ResolvedProjectTree projectTree, string archName) in VisualStudioUtils.EnumerateConfigs(ProjectService))
            {
                OnWriteFileConfig(writer, file, projectTree, archName);
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
            string filterPath = GetFilterPath(file.VirtualPath);
            if (!string.IsNullOrEmpty(filterPath))
            {
                writer.WriteElementString("Filter", filterPath);
            }
            writer.WriteEndElement();
        }

        foreach (FileEntry file in GeneratedFiles)
        {
            string fullPath = file.FullPath;
            string relPath = GetPath(fullPath);

            writer.WriteStartElement(GroupTag);
            writer.WriteAttributeString("Include", relPath);
            string filterPath = GetFilterPath(file.VirtualPath);
            if (!string.IsNullOrEmpty(filterPath))
            {
                writer.WriteElementString("Filter", filterPath);
            }
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

    protected static void HandleExcludedFile(XmlWriter writer, bool isExcludedFromBuild, ResolvedProjectTree projectTree, string archName)
    {
        if (isExcludedFromBuild)
        {
            string condition = VisualStudioUtils.GetConfigCondition(projectTree, archName);
            VisualStudioUtils.WriteElementString(writer, "ExcludedFromBuild", "true", condition);
        }
    }

    protected static bool TryGetFileConfig(FileEntry file, ResolvedProjectTree projectTree, out FileConfigEntry? config)
    {
        return file.TryGetConfig(projectTree, out config);
    }

    protected static ModuleNode GetConfigModule(FileConfigEntry config, ResolvedProjectTree projectTree)
    {
        if (projectTree.IndexedNodes.TryGetNode(config.Module.ModuleName, out ModuleNode? module))
        {
            return module;
        }

        throw new InvalidOperationException($"No module named '{config.Module.ModuleName}' was found in the resolved project tree for '{projectTree.ProjectContext.Platform.PlatformName}'.");
    }

    private static FileEntry GetOrAddFile(List<FileEntry> files, string filePath)
    {
        FileEntry? existing = files.FirstOrDefault((entry) => StringComparer.Ordinal.Equals(entry.FullPath, filePath));
        if (existing is not null)
        {
            return existing;
        }

        FileEntry added = new(filePath);
        files.Add(added);
        return added;
    }

    internal static string GetCommonParentDirectoryName(IReadOnlyList<FileEntry> files)
    {
        int commonStrLen = files[0].FullPath.Length;

        for (int i = 1; i < files.Count; i++)
        {
            FileEntry file = files[i];
            string fullPath = file.FullPath;

            commonStrLen = Math.Min(commonStrLen, fullPath.Length);

            for (int j = 0; j < commonStrLen; j++)
            {
                if (files[i].FullPath[j] != files[0].FullPath[j])
                {
                    commonStrLen = j;
                    break;
                }
            }
        }

        string commonStr = files[0].FullPath[..commonStrLen];

        // If it doesn't end with a path separator, then the last segment is a string prefix
        // of a file or directory name. We remove it to get the common ancestor directory path.
        if (!commonStr.EndsWith(Path.DirectorySeparatorChar))
        {
            string? dirName = Path.GetDirectoryName(commonStr);
            commonStr = string.IsNullOrEmpty(dirName) ? "" : dirName + Path.DirectorySeparatorChar;
        }

        return commonStr;
    }
}
