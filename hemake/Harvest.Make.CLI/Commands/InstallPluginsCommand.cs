// Copyright Chad Engler

using Harvest.Common;
using Harvest.Common.Attributes;
using Harvest.Common.Extensions;
using Harvest.Kdl;
using Harvest.Make.Projects;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.Logging;
using SharpCompress.Compressors.Xz;
using System.Buffers;
using System.Formats.Tar;
using System.IO.Compression;

namespace Harvest.Make.CLI.CliCommands;

[Command("Install plugins needed for the project.")]
internal partial class InstallPluginsCommand(
    ILogger<InstallPluginsCommand> logger,
    IProjectService projectService)
    : ICommandExecutor
{
    private const int DownloadProgressChunkBytes = 8 * 1024 * 1024;
    private const int DownloadProgressPercentStep = 10;

    public async Task<int> ExecuteAsync(CancellationToken ct)
    {
        List<InstallWorkItem> installItems = CollectInstallItems();
        if (installItems.Count == 0)
        {
            logger.LogInformation("No plugin archives need to be installed.");
            return 0;
        }

        logger.LogInformation("Installing {Count} plugin archives...", installItems.Count);

        using HttpClient httpClient = new();

        await Task.WhenAll(installItems.Select((item, index) =>
        {
            string progressPrefix = $"[{index + 1}/{installItems.Count}] {item.FetchNode.ArchivePrefix}";
            return InstallArchiveAsync(httpClient, item, progressPrefix, ct);
        }));

        logger.LogInformation("All plugins installed successfully.");
        return 0;
    }

    private List<InstallWorkItem> CollectInstallItems()
    {
        HashSet<string> installKeys = [];
        List<InstallWorkItem> installItems = [];

        foreach ((_, ResolvedProjectTree projectTree) in projectService.ResolvedProjectTrees)
        {
            foreach (PluginNode plugin in projectTree.GetNodes<PluginNode>(projectTree.ProjectNode.Node))
            {
                string installDir = projectTree.ProjectNode.InstallsDir;

                foreach (KdlNode fetchNodeKdl in plugin.Node.GetDescendantsByName(FetchNode.NodeTraits.Name))
                {
                    FetchNode fetchNode = new(fetchNodeKdl);
                    if (!installKeys.Add(fetchNode.ArchiveDirName))
                    {
                        continue;
                    }

                    installItems.Add(new(plugin.PluginName, fetchNode, installDir));
                }
            }
        }

        return installItems;
    }

    private async Task InstallArchiveAsync(HttpClient httpClient, InstallWorkItem item, string progressPrefix, CancellationToken ct)
    {
        string extractDir = Path.Combine(item.InstallDir, item.FetchNode.ArchiveDirName);

        if (Directory.Exists(extractDir))
        {
            logger.LogInformation("{ProgressPrefix} already installed.", progressPrefix);
            return;
        }

        string archiveTempPath = Path.Combine(Path.GetTempPath(), $"hemake-{Guid.NewGuid():N}.tmp");

        logger.LogInformation("{ProgressPrefix} installing for plugin '{PluginName}' from {ArchiveUri}", progressPrefix, item.PluginName, item.FetchNode.ArchiveUri);

        try
        {
            await DownloadArchiveAsync(httpClient, item.FetchNode, archiveTempPath, progressPrefix, ct);

            Directory.CreateDirectory(extractDir);
            await ExtractArchiveAsync(item.FetchNode, archiveTempPath, extractDir, progressPrefix, ct);

            logger.LogInformation("{ProgressPrefix} installed successfully.", progressPrefix);
        }
        catch (Exception ex)
        {
            try
            {
                Directory.Delete(extractDir, recursive: true);
            }
            catch (DirectoryNotFoundException)
            {
                // Ignore - we were trying to clean up from a failed install, so the directory may not exist
            }
            catch (Exception delEx)
            {
                logger.LogError(delEx, "Failed to clean up extract directory after failed install: {ExtractDir}", extractDir);
            }

            throw new InvalidOperationException($"Failed to install '{item.FetchNode.ArchiveDirName}' from '{item.FetchNode.ArchiveUri}'.", ex);
        }
        finally
        {
            try
            {
                File.Delete(archiveTempPath);
            }
            catch (FileNotFoundException)
            {
                // Ignore - the file may not have been created if the download failed early
            }
            catch (Exception ex)
            {
                logger.LogDebug(ex, "Failed to remove temp archive: {ArchiveTempPath}", archiveTempPath);
            }
        }
    }

    private async Task DownloadArchiveAsync(HttpClient httpClient, FetchNode fetchNode, string archivePath, string progressPrefix, CancellationToken ct)
    {
        using HttpResponseMessage response = await httpClient.GetAsync(fetchNode.ArchiveUri, HttpCompletionOption.ResponseHeadersRead, ct);
        response.EnsureSuccessStatusCode();

        long? totalBytes = response.Content.Headers.ContentLength;
        if (totalBytes.HasValue)
        {
            logger.LogInformation("{ProgressPrefix} downloading {SizeMiB:F1} KiB...", progressPrefix, totalBytes.Value / 1024.0);
        }
        else
        {
            logger.LogInformation("{ProgressPrefix} downloading...", progressPrefix);
        }

        await using Stream responseStream = await response.Content.ReadAsStreamAsync(ct);
        await using FileStream fileStream = new(archivePath, new FileStreamOptions()
        {
            Mode = FileMode.Create,
            Access = FileAccess.Write,
            Share = FileShare.None,
            Options = FileOptions.Asynchronous,
        });

        await CopyToAsyncWithProgress(responseStream, fileStream, totalBytes, progressPrefix, ct);
    }

    private async Task CopyToAsyncWithProgress(Stream source, Stream destination, long? totalBytes, string progressPrefix, CancellationToken ct)
    {
        using PooledBuffer<byte> buffer = ArrayPool<byte>.Shared.RentBuffer(81920);
        long totalRead = 0;
        int nextPercentToLog = DownloadProgressPercentStep;
        long nextBytesToLog = DownloadProgressChunkBytes;

        while (true)
        {
            int bytesRead = await source.ReadAsync(buffer, ct);
            if (bytesRead == 0)
            {
                break;
            }

            await destination.WriteAsync(buffer.AsMemory(..bytesRead), ct);
            totalRead += bytesRead;

            if (totalBytes is long expectedBytes && expectedBytes > 0)
            {
                int currentPercent = (int)((totalRead * 100) / expectedBytes);
                while (currentPercent >= nextPercentToLog && nextPercentToLog < 100)
                {
                    logger.LogInformation("{ProgressPrefix} download {Percent}% ({ReadMiB:F1} KiB)", progressPrefix, nextPercentToLog, totalRead / 1024.0);
                    nextPercentToLog += DownloadProgressPercentStep;
                }
            }
            else if (totalRead >= nextBytesToLog)
            {
                logger.LogInformation("{ProgressPrefix} downloaded {ReadMiB:F1} KiB", progressPrefix, totalRead / 1024.0);
                nextBytesToLog += DownloadProgressChunkBytes;
            }
        }

        await destination.FlushAsync(ct);
        logger.LogInformation("{ProgressPrefix} download complete ({ReadMiB:F1} KiB)", progressPrefix, totalRead / 1024.0);
    }

    private async Task ExtractArchiveAsync(FetchNode fetchNode, string archivePath, string extractDir, string progressPrefix, CancellationToken ct)
    {
        logger.LogInformation("{ProgressPrefix} extracting {ArchiveFormat} archive for {ArchiveDirName}...", progressPrefix, fetchNode.ArchiveFormat, fetchNode.ArchiveDirName);

        try
        {
            switch (fetchNode.ArchiveFormat)
            {
                case EFetchArchiveFormat.Zip:
                {
                    ZipFile.ExtractToDirectory(archivePath, extractDir, true);
                    break;
                }
                case EFetchArchiveFormat.Tar:
                {
                    await using FileStream tarStream = new(archivePath, new FileStreamOptions { Mode = FileMode.Open, Access = FileAccess.Read, Options = FileOptions.Asynchronous });
                    await ExtractTarArchiveAsync(tarStream, extractDir, progressPrefix, ct);
                    break;
                }
                case EFetchArchiveFormat.TarGz:
                {
                    await using FileStream tarStream = new(archivePath, new FileStreamOptions { Mode = FileMode.Open, Access = FileAccess.Read, Options = FileOptions.Asynchronous });
                    await using GZipStream gzipStream = new(tarStream, CompressionMode.Decompress, leaveOpen: true);
                    await ExtractTarArchiveAsync(gzipStream, extractDir, progressPrefix, ct);
                    break;
                }
                case EFetchArchiveFormat.TarXz:
                {
                    await using FileStream tarStream = new(archivePath, new FileStreamOptions { Mode = FileMode.Open, Access = FileAccess.Read, Options = FileOptions.Asynchronous });
                    await using XZStream xzStream = new(tarStream);
                    await ExtractTarArchiveAsync(xzStream, extractDir, progressPrefix, ct);
                    break;
                }
                default:
                {
                    throw new InvalidDataException($"Unsupported archive format '{fetchNode.ArchiveFormat}'.");
                }
            }
        }
        catch (Exception ex) when (ex is InvalidDataException or IOException)
        {
            long archiveSize = File.Exists(archivePath) ? new FileInfo(archivePath).Length : 0;
            throw new InvalidDataException($"Failed to extract '{fetchNode.ArchiveUri}' as '{fetchNode.ArchiveFormat}'. Downloaded file size: {archiveSize} bytes.", ex);
        }
    }

    private static async Task ExtractTarArchiveAsync(Stream archiveStream, string extractDir, string progressPrefix, CancellationToken ct)
    {
        await using TarReader tarReader = new(archiveStream);

        TarEntry? entry;
        while ((entry = await tarReader.GetNextEntryAsync(cancellationToken: ct)) is not null)
        {
            if (entry.EntryType is TarEntryType.RegularFile)
            {
                string entryOutputPath = Path.Combine(extractDir, entry.Name);
                Directory.CreateDirectory(Path.GetDirectoryName(entryOutputPath)!);
                await entry.ExtractToFileAsync(entryOutputPath, overwrite: true, ct);
            }
        }
    }

    private sealed record InstallWorkItem(string PluginName, FetchNode FetchNode, string InstallDir);
}
