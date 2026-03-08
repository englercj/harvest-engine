// Copyright Chad Engler

using Microsoft.Extensions.Logging;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace Harvest.Common.Services;

public partial class DotnetBuildService(ILogger<DotnetBuildService> logger) : IDotnetBuildService
{
    [GeneratedRegex(@"\s+->\s+(?<path>.+?\.dll)\s*$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant)]
    private static partial Regex BuiltAssemblyRegex();

    public async Task<DotnetBuildResult> BuildProjectAsync(string projectFilePath, CancellationToken ct = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(projectFilePath);

        if (!string.Equals(Path.GetExtension(projectFilePath), ".csproj", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException($"Unsupported project file '{projectFilePath}'. Only .csproj files are supported.");
        }

        if (!File.Exists(projectFilePath))
        {
            throw new FileNotFoundException($"Project file '{projectFilePath}' does not exist.", projectFilePath);
        }

        string dotnetPath = GetDotnetHostPath();
        string? projectDirectory = Path.GetDirectoryName(projectFilePath);

        ProcessStartInfo startInfo = new()
        {
            FileName = dotnetPath,
            WorkingDirectory = projectDirectory ?? Directory.GetCurrentDirectory(),
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
        };
        startInfo.ArgumentList.Add("build");
        startInfo.ArgumentList.Add(projectFilePath);
        startInfo.ArgumentList.Add("-c");
        startInfo.ArgumentList.Add("Release");
        startInfo.ArgumentList.Add("/nologo");
        startInfo.ArgumentList.Add("/verbosity:minimal");
        startInfo.ArgumentList.Add("/clp:NoSummary");

        string? builtAssemblyPath = null;
        List<string> outputLines = [];
        TaskCompletionSource stdOutClosed = new(TaskCreationOptions.RunContinuationsAsynchronously);
        TaskCompletionSource stdErrClosed = new(TaskCreationOptions.RunContinuationsAsynchronously);

        using Process process = new() { StartInfo = startInfo };
        process.OutputDataReceived += (_, e) =>
        {
            if (e.Data is null)
            {
                stdOutClosed.TrySetResult();
                return;
            }

            if (string.IsNullOrWhiteSpace(e.Data))
            {
                return;
            }

            lock (outputLines)
            {
                outputLines.Add(e.Data);
            }

            logger.LogDebug("{BuildOutput}", e.Data);
            TryCaptureAssemblyPath(e.Data, ref builtAssemblyPath);
        };
        process.ErrorDataReceived += (_, e) =>
        {
            if (e.Data is null)
            {
                stdErrClosed.TrySetResult();
                return;
            }

            if (string.IsNullOrWhiteSpace(e.Data))
            {
                return;
            }

            lock (outputLines)
            {
                outputLines.Add(e.Data);
            }

            logger.LogError("{BuildOutput}", e.Data);
            TryCaptureAssemblyPath(e.Data, ref builtAssemblyPath);
        };

        logger.LogInformation("Building HE Make extension project: {ProjectFile}", projectFilePath);

        process.Start();
        process.BeginOutputReadLine();
        process.BeginErrorReadLine();

        await process.WaitForExitAsync(ct);
        await Task.WhenAll(stdOutClosed.Task, stdErrClosed.Task);

        if (process.ExitCode != 0)
        {
            string failureDetails;
            lock (outputLines)
            {
                failureDetails = string.Join(Environment.NewLine, outputLines.TakeLast(20));
            }

            throw new DotnetBuildException(
                projectFilePath,
                string.IsNullOrWhiteSpace(failureDetails)
                    ? $"dotnet build exited with a non-zero exit code: {process.ExitCode}"
                    : $"dotnet build exited with a non-zero exit code: {process.ExitCode}{Environment.NewLine}{failureDetails}");
        }

        if (string.IsNullOrWhiteSpace(builtAssemblyPath))
        {
            throw new DotnetBuildException(projectFilePath, "Build completed successfully, but could not determine the output assembly path from build output.");
        }

        if (!Path.IsPathRooted(builtAssemblyPath))
        {
            builtAssemblyPath = Path.GetFullPath(builtAssemblyPath, projectDirectory ?? Directory.GetCurrentDirectory());
        }

        if (!File.Exists(builtAssemblyPath))
        {
            throw new DotnetBuildException(projectFilePath, $"Build completed successfully, but the output assembly does not exist: {builtAssemblyPath}");
        }

        return new(projectFilePath, builtAssemblyPath);
    }

    private static void TryCaptureAssemblyPath(string buildOutputLine, ref string? builtAssemblyPath)
    {
        Match match = BuiltAssemblyRegex().Match(buildOutputLine);
        if (!match.Success)
        {
            return;
        }

        string candidatePath = match.Groups["path"].Value.Trim();
        if (string.IsNullOrEmpty(candidatePath))
        {
            return;
        }

        builtAssemblyPath = candidatePath;
    }

    private static string GetDotnetHostPath()
    {
        string? dotnetHostPath = Environment.GetEnvironmentVariable("DOTNET_HOST_PATH");
        if (!string.IsNullOrWhiteSpace(dotnetHostPath))
        {
            return dotnetHostPath;
        }

        if (!string.IsNullOrWhiteSpace(Environment.ProcessPath)
            && Path.GetFileName(Environment.ProcessPath).StartsWith("dotnet", StringComparison.OrdinalIgnoreCase))
        {
            return Environment.ProcessPath!;
        }

        return "dotnet";
    }
}
