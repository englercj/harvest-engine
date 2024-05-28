// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Services;

// No service attribute because we register a special instance of this class directly in `Main()`.
//[Service<IProjectService>]
public class ProjectService : IProjectService
{
    private string _projectPath = string.Empty;
    private Dictionary<string, KdlDocument> _files = new();

    public ProjectService(INodeValidator)
    {
        return _files[path];
    }

    public void LoadProjectAsync(string projectPath)
    {
        _projectPath = projectPath;
        LoadFile(projectPath);
    }

    private void LoadFile(string path)
    {
        // FileStream h
        FileStreamOptions options = new()
        {
            Mode = FileMode.Open,
            Access = FileAccess.Read,
            BufferSize = 8192,
        };
        using FileStream reader = new(path, options);
        KdlDocument document = KdlDocument.From(reader);

        _files[path] = document;
        ValidateAndResolveImports(document);
    }

    private void ValidateAndResolveImports(KdlDocument document)
    {
        NodeValidator validator = new();
        foreach (KdlNode node in document.Nodes)
        {

            if (node.Name == "import")
            {
                string importPath = node.Arguments[0].Value;
                if (!Path.IsPathRooted(importPath))
                {
                    importPath = Path.Combine(Path.GetDirectoryName(_projectPath), importPath);
                }

                if (!_files.ContainsKey(importPath))
                {
                    LoadFile(importPath).Wait();
                }
            }
        }
    }
}
