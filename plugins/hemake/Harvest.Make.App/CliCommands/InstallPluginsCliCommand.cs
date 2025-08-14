// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.CliCommands;
using Harvest.Make.Projects;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.Logging;
using System.CommandLine.Invocation;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Formats.Tar;

namespace Harvest.Make.App.CliCommands;

[Service<ICliCommand>(Enumerable = true)]
internal class InstallPluginsCliCommand(
    ILogger<InstallPluginsCliCommand> logger,
    IProjectService projectService)
    : ICliCommand
{
    protected readonly ILogger _logger = logger;
    protected readonly IProjectService _projectService = projectService;

    public string Name => "install-plugins";
    public string Description => "Install plugins for the project.";

    public async Task<int> RunCommandAsync(InvocationContext context)
    {
        try
        {
            await InstallPluginsAsync(context);
            return 0;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "An error occurred while installing plugins.");
            return -1;
        }
    }

    private async Task InstallPluginsAsync(InvocationContext invocationContext)
    {
        ProjectContext baseContext = _projectService.CreateProjectContext(invocationContext);
        List<PluginNode> plugins = _projectService.GetNodes<PluginNode>(baseContext);
        List<ConfigurationNode> configurations = _projectService.GetNodes<ConfigurationNode>(baseContext);
        List<PlatformNode> platforms = _projectService.GetNodes<PlatformNode>(baseContext);

        List<Task> installTasks = [];

        foreach (ConfigurationNode configuration in configurations)
        {
            foreach (PlatformNode platform in platforms)
            {
                ProjectContext context = _projectService.CreateProjectContext(invocationContext, null, configuration, platform);

                // TODO: install node may have unscoped install logic, which means we'd duplicate
                // the install logic for each configuration/platform combination.
                // Maybe we should try to get an install from baseContext first, then store each
                // install in a dictionary and only run the install once.
                foreach (PluginNode plugin in plugins)
                {
                    installTasks.Add(InstallPluginAsync(context, plugin));
                }
            }
        }

        await Task.WhenAll(installTasks);

        _logger.LogInformation("All plugins installed successfully.");
    }

    private async Task InstallPluginAsync(ProjectContext context, PluginNode plugin)
    {
        string dirName = Path.GetDirectoryName(plugin.Node.SourceInfo.FilePath) ?? string.Empty;
        plugin.InstallDir = dirName;

        foreach (ArchiveNode archive in _projectService.GetNodes<ArchiveNode>(context, plugin, false))
        {

        }
    }

    private async Task<string> InstallFromArchive(string name, string url, string archiveName, string? extractDirName, string pluginsDir)
    {
        string digest = ComputeDigest(name + url);
        string archiveDir = Path.Combine(pluginsDir, name);

        string digestFilePath = Path.Combine(archiveDir, ".he_plugin_digest");
        string installedVersion = string.Empty;
        try
        {
            installedVersion = await File.ReadAllTextAsync(digestFilePath);
        }
        catch { }

        string extractDir = string.IsNullOrEmpty(extractDirName) ? archiveDir : Path.Combine(archiveDir, extractDirName);
        extractDir = Uri.UnescapeDataString(extractDir);

        if (installedVersion == digest && Directory.Exists(extractDir))
        {
            _logger.LogTrace("Plugin {PluginName} is already installed and up to date.", name);
            return extractDir;
        }

        _logger.LogTrace("Installing plugin {PluginName} from archive: {Url}", name, url);

        string archivePath = Path.Combine(archiveDir, archiveName);
        Directory.CreateDirectory(archiveDir);

        using HttpClient httpClient = new();
        using HttpResponseMessage response = await httpClient.GetAsync(url);
        response.EnsureSuccessStatusCode();
        using FileStream fs = new(archivePath, FileMode.Create, FileAccess.Write, FileShare.None);
        await response.Content.CopyToAsync(fs);

        Directory.CreateDirectory(extractDir);

        if (archiveName.EndsWith(".zip", StringComparison.OrdinalIgnoreCase))
        {
            ZipFile.ExtractToDirectory(archivePath, extractDir, true);
        }
        else if (archiveName.EndsWith(".tar.gz", StringComparison.OrdinalIgnoreCase)
            || archiveName.EndsWith(".tgz", StringComparison.OrdinalIgnoreCase))
        {
            await using FileStream tarStream = new(archivePath, new FileStreamOptions { Mode = FileMode.Open, Access = FileAccess.Read, Options = FileOptions.Asynchronous });
            await using GZipStream gzipStream = new(tarStream, CompressionMode.Decompress, leaveOpen: true);
            await using TarReader tarReader = new(gzipStream);
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
        else
        {
            throw new NotSupportedException($"Unsupported archive format: {archiveName}");
        }

        File.WriteAllText(digestFilePath, digest);
        return extractDir;
    }

    private static string ComputeDigest(string input)
    {
        byte[] inputBytes = Encoding.UTF8.GetBytes(input);
        byte[] hashBytes = SHA256.HashData(inputBytes);
        return Convert.ToHexString(hashBytes);
    }
}
