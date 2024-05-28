// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

// No service attribute because we register a special instance of this class directly in `Main()`.
//[Service<IProjectService>]
public class ProjectService : IProjectService
{
    private Dictionary<string, Type> _nodeTypes = new();

    private string _projectPath = string.Empty;
    private Dictionary<string, KdlDocument> _files = new();

    private List<INode> _nodes = new();

    public void RegisterNode<T>() where T : INode
    {
        T node = Activator.CreateInstance<T>();
        _nodeTypes.Add(node.Name, typeof(T));
    }

    public void LoadProjectAsync(string projectPath)
    {
        _projectPath = projectPath;
        LoadAndValidateFile(projectPath, null);
    }

    private void LoadAndValidateFile(string path, INode? scope)
    {
        // FileStream uses a small buffer by default, so we increase it to 8k which is
        // large enough for most files we're loading.
        FileStreamOptions options = new()
        {
            Mode = FileMode.Open,
            Access = FileAccess.Read,
            BufferSize = 8192,
        };
        using FileStream reader = new(path, options);
        KdlDocument document = KdlDocument.From(reader);

        _files[path] = document;

        foreach (KdlNode node in document.Nodes)
        {
            ValidateAndResolve(path, node, scope);
        }
    }

    private INode ValidateAndResolve(string filePath, KdlNode rawNode, INode? scope)
    {
        if (!_nodeTypes.TryGetValue(rawNode.Name, out Type? nodeType))
        {
            throw new Exception($"Project invalid. Unknown node '{rawNode.Name}' in file: {filePath}");
        }

        INode? node = Activator.CreateInstance(nodeType, rawNode) as INode;
        if (node is null)
        {
            throw new Exception($"Project invalid. Failed to create node instance for '{rawNode.Name}' in file: {filePath}");
        }

        NodeValidationResult result = node.Validate(scope);
        if (!result.IsValid)
        {
            throw new Exception($"Project invalid. Validation failed: {result.ErrorContent}");
        }

        foreach (KdlNode rawChild in rawNode.Children)
        {
            INode child = ValidateAndResolve(filePath, rawChild, node);

            if (child is not ImportNode)
            {
                node.Children.Add(child);
                continue;
            }

            string? importPath = ((ImportNode)child).ImportPath;
            if (importPath is null)
            {
                continue;
            }

            if (!Path.IsPathRooted(importPath))
            {
                string? directory = Path.GetDirectoryName(filePath);
                if (directory is not null)
                {
                    importPath = Path.Combine(directory, importPath);
                }
            }

            if (!_files.ContainsKey(importPath))
            {
                LoadAndValidateFile(importPath, node);
            }
        }

        return node;
    }
}
