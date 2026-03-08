// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using System.Text;

namespace Harvest.Make.Extensions.Common;

public static class ExtensionNodeUtils
{
    public static KdlNode GetOwningModule(KdlNode node)
    {
        KdlNode? current = node.Parent;
        while (current is not null && current.Name != ModuleNode.NodeTraits.Name)
        {
            current = current.Parent;
        }

        return current ?? throw new NodeParseException(node, "Failed to find the owning module.");
    }

    public static KdlNode GetOwningPlugin(KdlNode node)
    {
        KdlNode? current = node.Parent;
        while (current is not null && current.Name != PluginNode.NodeTraits.Name)
        {
            current = current.Parent;
        }

        return current ?? throw new NodeParseException(node, "Failed to find the owning plugin.");
    }

    public static int GetLexicalIndex(KdlNode ownerModule, KdlNode sourceNode, string nodeName)
    {
        int index = 0;
        foreach (KdlNode child in ownerModule.GetAllDescendants())
        {
            if (child.Name != nodeName)
            {
                continue;
            }

            ++index;
            if (ReferenceEquals(child, sourceNode))
            {
                return index;
            }
        }

        throw new NodeParseException(sourceNode, $"Failed to determine the lexical index for '{nodeName}'.");
    }

    public static T GetRequiredNode<T>(KdlNode parent, string nodeName)
        where T : class, INode
    {
        T? node = GetOptionalNode<T>(parent, nodeName);
        if (node is null)
        {
            throw new NodeParseException(parent, $"'{parent.Name}' nodes require a '{nodeName}' child.");
        }

        return node;
    }

    public static T? GetOptionalNode<T>(KdlNode parent, string nodeName)
        where T : class, INode
    {
        List<KdlNode> matchingChildren = [.. parent.Children.Where((child) => child.Name == nodeName)];
        if (matchingChildren.Count > 1)
        {
            throw new NodeParseException(parent, $"'{parent.Name}' nodes may only contain one '{nodeName}' child.");
        }

        return matchingChildren.Count == 0 ? null : INodeTraits.CreateNode<T>(matchingChildren[0]);
    }

    public static KdlNode GetSourceModule(IProjectService projectService, string moduleName)
    {
        foreach (KdlNode moduleNode in projectService.ProjectDocument.GetNodesByName(ModuleNode.NodeTraits.Name))
        {
            if (moduleNode.TryGetValue(0, out string? currentModuleName)
                && string.Equals(currentModuleName, moduleName, StringComparison.Ordinal))
            {
                return moduleNode;
            }
        }

        throw new NodeParseException(new KdlNode(moduleName), $"No module named '{moduleName}' was found.");
    }

    public static string GetModuleRelativePath(KdlNode ownerSourceModule, string fullPath)
    {
        string moduleDir = Path.GetDirectoryName(ownerSourceModule.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();
        return Path.GetRelativePath(moduleDir, fullPath).Replace('\\', '/');
    }

    public static string GetGeneratedDirToken(string ownerModuleName, string relativePath)
    {
        string normalized = relativePath.Replace('\\', '/').TrimStart('/');
        return string.IsNullOrEmpty(normalized)
            ? $"${{module[{ownerModuleName}].gen_dir}}"
            : $"${{module[{ownerModuleName}].gen_dir}}/{normalized}";
    }

    public static string SanitizeIdentifier(string value)
    {
        StringBuilder builder = new(value.Length);
        foreach (char ch in value)
        {
            builder.Append(char.IsLetterOrDigit(ch) ? ch : '_');
        }

        return builder.ToString();
    }

    public static bool TryGetContainingRoot(string fullPath, IEnumerable<string> roots, out string containingRoot, out string relativePath)
    {
        foreach (string root in roots.OrderByDescending((string value) => value.Length))
        {
            string rootPath = Path.GetFullPath(root);
            string fullFilePath = Path.GetFullPath(fullPath);
            string candidate = Path.GetRelativePath(rootPath, fullFilePath);
            if (candidate == ".")
            {
                containingRoot = rootPath;
                relativePath = string.Empty;
                return true;
            }

            if (!candidate.StartsWith("..", GetPathComparison()) && !Path.IsPathRooted(candidate))
            {
                containingRoot = rootPath;
                relativePath = candidate.Replace('\\', '/');
                return true;
            }
        }

        containingRoot = string.Empty;
        relativePath = string.Empty;
        return false;
    }

    public static void AddOwnerDependency(KdlNode ownerTarget, KdlSourceInfo sourceInfo, string dependencyName, bool isPublic, NodeResolver resolver, string? kind = null)
    {
        KdlNode dependencyNode = CreateDependenciesNode(sourceInfo, [CreateDependencyEntry(dependencyName, sourceInfo, kind)]);
        if (isPublic)
        {
            KdlNode publicNode = CreateNode(PublicNode.NodeTraits.Name, sourceInfo);
            publicNode.AddChild(dependencyNode);
            ownerTarget.AddChild(resolver.CreateResolvedNode(publicNode, includeChildren: true));
        }
        else
        {
            ownerTarget.AddChild(resolver.CreateResolvedNode(dependencyNode, includeChildren: true));
        }
    }

    public static KdlNode CreateDependenciesNode(KdlSourceInfo sourceInfo, IEnumerable<KdlNode> entries)
    {
        KdlNode node = CreateNode(DependenciesNode.NodeTraits.Name, sourceInfo);
        foreach (KdlNode entry in entries)
        {
            node.AddChild(entry);
        }

        return node;
    }

    public static KdlNode CreateStringEntriesNode(string nodeName, KdlSourceInfo sourceInfo, IEnumerable<string> entries)
    {
        KdlNode node = CreateNode(nodeName, sourceInfo);
        foreach (string entry in entries.Where((value) => !string.IsNullOrWhiteSpace(value)).Distinct(StringComparer.Ordinal))
        {
            node.AddChild(CreateNode(entry, sourceInfo));
        }

        return node;
    }

    public static KdlNode CreateDependencyEntry(string dependencyName, KdlSourceInfo sourceInfo, string? kind = null)
    {
        Dictionary<string, KdlValue>? properties = null;
        if (!string.IsNullOrEmpty(kind))
        {
            properties = new Dictionary<string, KdlValue>()
            {
                ["kind"] = new KdlString(kind),
            };
        }

        return CreateNode(dependencyName, sourceInfo, properties: properties);
    }

    public static KdlNode CreateBuildRuleNode(KdlSourceInfo sourceInfo, string ruleName, string message, IEnumerable<string> commands, IEnumerable<string> inputs, IEnumerable<string> outputs, bool linkOutput)
    {
        KdlNode buildRuleNode = CreateNode(BuildRuleNode.NodeTraits.Name, sourceInfo,
            new KdlString(ruleName),
            properties: new Dictionary<string, KdlValue>()
            {
                ["message"] = new KdlString(message),
                ["link_output"] = new KdlBool(linkOutput),
            });

        foreach (string command in commands)
        {
            buildRuleNode.AddChild(CreateNode(CommandNode.NodeTraits.Name, sourceInfo, new KdlString(command)));
        }

        buildRuleNode.AddChild(CreateStringEntriesNode(InputsNode.NodeTraits.Name, sourceInfo, inputs));
        buildRuleNode.AddChild(CreateStringEntriesNode(OutputsNode.NodeTraits.Name, sourceInfo, outputs));
        return buildRuleNode;
    }

    public static KdlNode CreateNode(string name, KdlSourceInfo sourceInfo, params KdlValue[] arguments)
    {
        return CreateNode(name, sourceInfo, arguments, null);
    }

    public static KdlNode CreateNode(string name, KdlSourceInfo sourceInfo, IReadOnlyDictionary<string, KdlValue>? properties)
    {
        return CreateNode(name, sourceInfo, [], properties);
    }

    public static KdlNode CreateNode(string name, KdlSourceInfo sourceInfo, KdlValue argument, IReadOnlyDictionary<string, KdlValue>? properties)
    {
        return CreateNode(name, sourceInfo, [argument], properties);
    }

    public static KdlNode CreateNode(string name, KdlSourceInfo sourceInfo, IEnumerable<KdlValue> arguments, IReadOnlyDictionary<string, KdlValue>? properties)
    {
        KdlNode node = new(name)
        {
            SourceInfo = sourceInfo,
        };

        foreach (KdlValue argument in arguments)
        {
            node.Arguments.Add(argument);
        }

        if (properties is not null)
        {
            foreach ((string key, KdlValue value) in properties)
            {
                node.Properties[key] = value;
            }
        }

        return node;
    }

    private static StringComparison GetPathComparison()
    {
        return OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal;
    }
}
