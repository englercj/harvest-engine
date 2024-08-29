// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Attributes;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

[Service<IProjectService>]
public class ProjectService : IProjectService
{
    private readonly Dictionary<string, Type> _nodeTypes = [];
    private readonly Dictionary<string, KdlDocument> _files = [];

    private string _projectPath = string.Empty;
    public string ProjectPath => _projectPath;

    public ProjectNode? Project { get; private set; }

    private List<string> _configurations = [];
    public IReadOnlyList<string> Configurations => _configurations;

    private List<string> _platforms = [];
    public IReadOnlyList<string> Platforms => _platforms;

    public void RegisterNode<T>(string name) where T : INode
    {
        _nodeTypes.Add(name, typeof(T));
    }

    public void LoadProject(string projectPath)
    {
        _projectPath = projectPath;

        KdlDocument document = LoadFile(projectPath);

        if (document.Nodes.Count != 1 || document.Nodes[0].Name != ProjectNode.NodeName)
        {
            throw new Exception($"Project invalid. Expected a single root '{ProjectNode.NodeName}' node in file: {projectPath}");
        }

        Project = (ProjectNode)CreateAndValidateNode(projectPath, document.Nodes[0], null);
    }

    private KdlDocument LoadFile(string path)
    {
        // FileStream uses a small buffer by default, so we increase it to 4k which should be
        // large enough for just about any of the project files we're loading. Also happens to
        // match the common OS page size, which might mean something; who knows.
        FileStreamOptions options = new()
        {
            Mode = FileMode.Open,
            Access = FileAccess.Read,
            BufferSize = 4096,
        };
        using FileStream reader = new(path, options);
        KdlDocument document = KdlDocument.From(reader);

        _files[path] = document;
        return document;
    }

    private INode CreateAndValidateNode(string filePath, KdlNode rawNode, INode? scope)
    {
        INode? node;

        if (scope is not null && scope.ChildNodeType is not null)
        {
            if (Activator.CreateInstance(scope.ChildNodeType, rawNode) is not INode instance)
            {
                throw new Exception($"Project invalid. Failed to create node instance for '{rawNode.Name}' in file: {filePath}");
            }

            node = instance;
        }
        else
        {
            // Remove the plus from extension nodes so we can find the type by name.
            string nodeName = rawNode.Name;
            if (nodeName.StartsWith('+'))
            {
                nodeName = nodeName[1..];
            }

            if (!_nodeTypes.TryGetValue(nodeName, out Type? nodeType))
            {
                throw new Exception($"Project invalid. Unknown node '{nodeName}' in file: {filePath}");
            }

            if (Activator.CreateInstance(nodeType, rawNode) is not INode instance)
            {
                throw new Exception($"Project invalid. Failed to create node instance for '{nodeName}' in file: {filePath}");
            }

            node = instance;
            foreach (KdlNode rawChild in rawNode.Children)
            {
                INode child = CreateAndValidateNode(filePath, rawChild, node);

                if (child is ImportNode importNode)
                {
                    ResolveImport(filePath, importNode, node);
                }
                else if (child is ConfigurationNode configurationNode)
                {
                    _configurations.Add(configurationNode.Name);
                }
                else if (child is PlatformNode platformNode)
                {
                    _platforms.Add(platformNode.Name);
                }
                else
                {
                    node.Children.Add(child);
                }
            }
        }

        NodeValidationResult result = node.Validate(scope);
        if (!result.IsValid)
        {
            throw new Exception($"Project invalid. Validation failed for '{node.Name}' node: {result.ErrorContent}\n    in file: {filePath}");
        }

        return node;
    }

    private void ResolveImport(string filePath, ImportNode child, INode scope)
    {
        string? importPath = child.ImportPath;
        if (importPath is null)
        {
            return;
        }

        if (!Path.IsPathRooted(importPath))
        {
            string? directory = Path.GetDirectoryName(filePath);
            if (directory is not null)
            {
                importPath = Path.Combine(directory, importPath);
            }
        }

        if (_files.ContainsKey(importPath))
        {
            return;
        }

        KdlDocument document = LoadFile(importPath);

        foreach (KdlNode rawNode in document.Nodes)
        {
            INode importedNode = CreateAndValidateNode(importPath, rawNode, scope);
            scope.Children.Add(importedNode);
        }
    }
}
