// Copyright Chad Engler

using Microsoft.Extensions.Logging;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Common.Services;

public readonly record struct DotnetBuildResult(string ProjectFilePath, string AssemblyPath);

public class DotnetBuildException : Exception
{
    public string ProjectFilePath { get; }

    public DotnetBuildException(string projectFilePath, string message) : base(message)
    {
        ProjectFilePath = projectFilePath;
    }

    public DotnetBuildException(Exception innerException, string projectFilePath, string message) : base(message, innerException)
    {
        ProjectFilePath = projectFilePath;
    }
}

public interface IDotnetBuildService
{
    Task<DotnetBuildResult> BuildProjectAsync(string projectFilePath, CancellationToken ct = default);
}
