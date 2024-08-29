// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public interface IProjectService
{
    public void RegisterNode<T>(string name) where T : INode;

    public void LoadProject(string projectPath);
}
