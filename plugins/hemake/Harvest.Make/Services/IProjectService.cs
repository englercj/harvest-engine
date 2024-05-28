// Copyright Chad Engler

namespace Harvest.Make.Services;

public interface IProjectService
{
    public Task LoadProjectAsync(string projectPath);
}
