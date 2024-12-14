// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Attributes;
using Harvest.Make.Projects.Nodes;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects;

[Service<IProjectService>]
public class ProjectService : IProjectService
{
    private readonly Dictionary<string, Type> _nodeTypes = [];
    private readonly Dictionary<string, KdlDocument> _files = [];

    public string ProjectPath { get; private set; } = "";
    public ProjectNode? Project { get; private set; }
    public List<ProjectOption> Options { get; } = [];

    public void RegisterNodeType<T>() where T : class, INode
    {
        _nodeTypes.Add(NodeTraits<T>.Name, typeof(T));
    }

    public void LoadProject(string projectPath)
    {
        if (!Path.IsPathRooted(projectPath))
        {
            projectPath = Path.GetFullPath(projectPath);
        }

        ProjectPath = projectPath;

        KdlDocument document = LoadFile(projectPath);

        if (document.Nodes.Count != 1 || document.Nodes[0].Name != ProjectNode.NodeName)
        {
            throw new Exception($"Project invalid. Expected a single root '{ProjectNode.NodeName}' node in file: {projectPath}");
        }

        Project = (ProjectNode)CreateAndValidateNode(projectPath, document.Nodes[0], null);

        ProjectContext context = GetProjectContext();
        Options.Clear();
        foreach (OptionNode optionNode in FindNodes<OptionNode>(context))
        {
            Options.Add(new ProjectOption(optionNode, optionNode.OptionType switch
            {
                EOptionType.Bool => new Option<bool?>("--" + optionNode.OptionName, () => optionNode.GetDefaultBool(), optionNode.HelpText),
                EOptionType.Int => new Option<long?>("--" + optionNode.OptionName, () => optionNode.GetDefaultNumber<long>(), optionNode.HelpText),
                EOptionType.UInt => new Option<ulong?>("--" + optionNode.OptionName, () => optionNode.GetDefaultNumber<ulong>(), optionNode.HelpText),
                EOptionType.Float => new Option<double?>("--" + optionNode.OptionName, () => optionNode.GetDefaultNumber<double>(), optionNode.HelpText),
                EOptionType.String => new Option<string?>("--" + optionNode.OptionName, () => optionNode.GetDefaultString(), optionNode.HelpText),
                _ => throw new Exception($"Unknown option type: {optionNode.OptionType}")
            }));
        }
    }

    public ProjectContext GetProjectContext(InvocationContext? invocationContext = null, ModuleNode? module = null, ConfigurationNode? configuration = null, PlatformNode? platform = null)
    {
        ProjectContext context = new();

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            context.Host = EPlatformSystem.Windows;
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            context.Host = EPlatformSystem.Linux;
        }
        else
        {
            throw new Exception("Unsupported host platform.");
        }

        if (invocationContext is not null)
        {
            foreach (ProjectOption projectOption in Options)
            {
                object? value = invocationContext.ParseResult.GetValueForOption(projectOption.Option);
                if (value is null && projectOption.Node.EnvVarName is not null)
                {
                    value = Environment.GetEnvironmentVariable(projectOption.Node.EnvVarName);
                }

                if (value is null)
                    continue;

                context.Options.Add(projectOption.Node.OptionName, value);
            }
        }

        if (module is not null)
        {
            context.Language = module.Language;
        }

        if (configuration is not null)
        {
            context.Configuration = configuration.ConfigName;
        }

        if (platform is not null)
        {
            context.Arch = platform.Arch;
            context.Platform = platform.PlatformName;
            context.System = platform.System;
            context.Toolset = platform.Toolset;
        }

        // This is done in a loop because tags may be activated by other tags. That is, it is valid
        // to write: `tags { a } when tags=a { tags { b } }`. In this case, the first loop will
        // activate `a` but not `b` because in the first loop iteration no tags are active.
        // Then the second loop will activate `b` because `a` was activated by the first iteration.
        // Once we encounter a loop iteration where no additional tags are activated, we are done.
        bool tagsChanged = true;
        while (tagsChanged)
        {
            tagsChanged = false;

            TagsNode tagsNode = GetResolvedNode<TagsNode>(context, module);
            foreach (TagsEntryNode entry in tagsNode.Entries)
            {
                tagsChanged |= context.Tags.Add(entry.Tag);
            }
        }

        return context;
    }

    public List<ConfigurationNode> GetDefaultConfigurations()
    {
        KdlNode debugNode = new("configuration");
        debugNode.Arguments.Add(new KdlString("Debug"));

        KdlNode releaseNode = new("configuration");
        releaseNode.Arguments.Add(new KdlString("Release"));

        return
        [
            new ConfigurationNode(debugNode, Project),
            new ConfigurationNode(releaseNode, Project),
        ];
    }

    public List<PlatformNode> GetDefaultPlatforms()
    {
        string arch = RuntimeInformation.OSArchitecture switch
        {
            Architecture.X86 => "x86",
            Architecture.X64 => "x86_64",
            Architecture.Arm => "arm",
            Architecture.Arm64 => "arm64",
            _ => throw new Exception("Unsupported architecture.")
        };

        KdlNode node = new("platform");
        node.Properties.Add("arch", new KdlString(arch));

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            string name = RuntimeInformation.OSArchitecture switch
            {
                Architecture.X86 => "Win32",
                Architecture.X64 => "Win64",
                Architecture.Arm => "WinArm32",
                Architecture.Arm64 => "WinArm64",
                _ => throw new Exception("Unsupported architecture.")
            };

            node.Arguments.Add(new KdlString(name));
            node.Properties.Add("system", new KdlString("windows"));
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            string name = RuntimeInformation.OSArchitecture switch
            {
                Architecture.X86 => "Linux32",
                Architecture.X64 => "Linux64",
                Architecture.Arm => "LinuxArm32",
                Architecture.Arm64 => "LinuxArm64",
                _ => throw new Exception("Unsupported architecture.")
            };

            node.Arguments.Add(new KdlString(name));
            node.Properties.Add("system", new KdlString("linux"));
        }
        else
        {
            throw new Exception("Unsupported host platform.");
        }

        return [new PlatformNode(node, Project)];
    }

    public IEnumerable<T> FindNodes<T>(ProjectContext context, INode? scope = null) where T : class, INode
    {
        scope ??= Project;

        Stack<INode> stack = new();

        while (scope is not null)
        {
            stack.Clear();
            stack.Push(scope);

            // Search the scope children, descending into any when nodes we encounter
            while (stack.Count > 0)
            {
                INode s = stack.Pop();
                foreach (INode child in s.Children)
                {
                    if (child is T instance)
                    {
                        yield return instance;
                    }
                    else if (child.Name == WhenNode.NodeName)
                    {
                        WhenNode whenNode = (WhenNode)child;
                        if (whenNode.IsActive(context))
                        {
                            stack.Push(whenNode);
                        }
                    }
                }
            }

            // Walk up the scope chain, but only check scopes where the node is allowed
            do
            {
                scope = scope.Scope;
            } while (scope is not null && !NodeTraits<T>.Scopes.Contains(scope.Name));
        }
    }

    public T GetResolvedNode<T>(ProjectContext context, INode? scope = null) where T : class, INode
    {
        return GetResolvedNode<T>(context, scope, (v) => true);
    }

    public T GetResolvedNode<T>(ProjectContext context, INode? scope, Func<T, bool> filter) where T : class, INode
    {
        bool isSet = ReflectionUtils.IsInstanceOfGenericType<T>(typeof(NodeSetBase<>));
        string name = NodeTraits<T>.Name;
        KdlNode result = new(name);

        if (Activator.CreateInstance(typeof(T), result, scope) is not T resolved)
        {
            throw new Exception($"Failed to allocate resolved node {name}.");
        }

        foreach (T node in FindNodes<T>(context, scope).Reverse().Where(filter))
        {
            resolved.MergeAndResolve(context, node);
        }

        return resolved;
    }

    private KdlDocument LoadFile(string path)
    {
        if (!Path.IsPathRooted(path))
        {
            throw new ArgumentException($"Path must be absolute: {path}", nameof(path));
        }

        if (_files.TryGetValue(path, out KdlDocument? document))
        {
            return document;
        }

        return _files[path] = KdlDocument.FromFile(path);
    }

    private INode CreateAndValidateNode(string filePath, KdlNode rawNode, INode? scope)
    {
        INode? node;

        if (scope is not null && scope.ChildNodeType is not null)
        {
            if (Activator.CreateInstance(scope.ChildNodeType, rawNode, scope) is not INode instance)
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

            // TODO: generators (':'), and extensions ('+')

            if (!_nodeTypes.TryGetValue(nodeName, out Type? nodeType))
            {
                throw new Exception($"Project invalid. Unknown node '{nodeName}' in file: {filePath}");
            }

            if (Activator.CreateInstance(nodeType, rawNode, scope) is not INode instance)
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

        KdlDocument document = LoadFile(importPath);

        foreach (KdlNode rawNode in document.Nodes)
        {
            INode importedNode = CreateAndValidateNode(importPath, rawNode, scope);
            scope.Children.Add(importedNode);
        }
    }
}
