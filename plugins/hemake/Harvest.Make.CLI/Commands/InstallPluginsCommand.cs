// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.CliCommands;
using Harvest.Make.Projects;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.Logging;
using System.CommandLine.Invocation;
using System.Formats.Tar;
using System.IO.Compression;

namespace Harvest.Make.CLI.CliCommands;

[Command("Install plugins needed for the project.")]
internal partial class InstallPluginsCliCommand(
    ILogger<InstallPluginsCliCommand> logger,
    IProjectService projectService)
    : ICommandExecutor
{
    public async Task<int> ExecuteAsync(CancellationToken ct)
    {
        HashSet<string> installKeys = [];
        List<Task> installTasks = [];

        using HttpClient httpClient = new();

        foreach ((_, ResolvedProjectTree projectTree) in _projectService.ResolvedProjectTrees)
        {
            foreach (PluginNode plugin in projectTree.GetNodes<PluginNode>(projectTree.ProjectNode.Node))
            {
                string installDir = projectTree.ProjectNode.InstallsDir;

                foreach (FetchNode fetchNode in projectTree.GetNodes<FetchNode>(plugin.Node))
                {
                    if (installKeys.Add(fetchNode.ArchiveKey))
                    {
                        installTasks.Add(InstallArchiveAsync(httpClient, fetchNode, installDir));
                    }
                }
            }
        }

        // TODO: Download progress reporting?

        await Task.WhenAll(installTasks);

        // TODO: How do we clean up old/unneeded plugin installs?
        // If you update a plugin on a branch, it can be pretty annoying to have it removed/added
        // every time you switch branches. Maybe we need a separate "prune" command or something?

        _logger.LogInformation("All plugins installed successfully.");
        return 0;
    }

    private async Task InstallArchiveAsync(HttpClient httpClient, FetchNode fetchNode, string installDir)
    {
        string extractDir = Path.Combine(installDir, fetchNode.ArchiveKey);

        if (Directory.Exists(extractDir))
        {
            _logger.LogTrace("Archive is already installed: {ArchiveUrl}", fetchNode.ArchiveUrl);
            return;
        }

        string archiveTempPath = Path.GetTempFileName();

        await DownloadArchiveAsync(httpClient, fetchNode.ArchiveUrl, archiveTempPath);

        try
        {
            Directory.CreateDirectory(extractDir);

            await ExtractArchiveAsync(fetchNode.ArchiveFormat, archiveTempPath, extractDir);
        }
        catch (Exception)
        {
            // Remove the extract dir on failure to avoid leaving a partial install
            try
            {
                Directory.Delete(extractDir, recursive: true);
            }
            catch (Exception delEx)
            {
                // Log but ignore errors during cleanup, they are secondary to the original error
                _logger.LogError(delEx, "Failed to clean up extract directory after failed install: {ExtractDir}", extractDir);
            }

            // Rethrow the original exception
            throw;
        }
    }

    private async Task DownloadArchiveAsync(HttpClient httpClient, string archiveUrl, string archivePath)
    {
        _logger.LogTrace("Downloading archive: {Url}", archiveUrl);

        using HttpResponseMessage response = await httpClient.GetAsync(archiveUrl);
        response.EnsureSuccessStatusCode();
        await using FileStream fs = new(archivePath, FileMode.Create, FileAccess.Write, FileShare.None);
        await response.Content.CopyToAsync(fs);
    }

    private static async Task ExtractArchiveAsync(EFetchArchiveFormat archiveFormat, string archivePath, string extractDir)
    {
        switch (archiveFormat)
        {
            case EFetchArchiveFormat.Zip:
            {
                ZipFile.ExtractToDirectory(archivePath, extractDir, true);
                break;
            }
            case EFetchArchiveFormat.Tar:
            {
                await using FileStream tarStream = new(archivePath, new FileStreamOptions { Mode = FileMode.Open, Access = FileAccess.Read, Options = FileOptions.Asynchronous });
                await ExtractTarArchiveAsync(tarStream, extractDir);
                break;
            }
            case EFetchArchiveFormat.TarGz:
            {
                await using FileStream tarStream = new(archivePath, new FileStreamOptions { Mode = FileMode.Open, Access = FileAccess.Read, Options = FileOptions.Asynchronous });
                await using GZipStream gzipStream = new(tarStream, CompressionMode.Decompress, leaveOpen: true);
                await ExtractTarArchiveAsync(gzipStream, extractDir);
                break;
            }
        }
    }

    private static async Task ExtractTarArchiveAsync(Stream archiveStream, string extractDir)
    {
        await using TarReader tarReader = new(archiveStream);

        TarEntry? entry;
        while ((entry = await tarReader.GetNextEntryAsync()) is not null)
        {
            if (entry.EntryType is TarEntryType.RegularFile)
            {
                string entryOutputPath = Path.Combine(extractDir, entry.Name);
                Directory.CreateDirectory(Path.GetDirectoryName(entryOutputPath)!);
                await entry.ExtractToFileAsync(entryOutputPath, overwrite: true);
            }
        }
    }
}
