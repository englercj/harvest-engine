// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Attributes;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.FileSystemGlobbing;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects;

[Service<IProjectService>]
public class ProjectService : IProjectService
{
    private readonly Dictionary<string, Type> _nodeTypes = [];
    private readonly Dictionary<string, INodeTraits> _nodeTraits = [];

    private readonly Dictionary<string, Type> _nodeGenerators = [];

    private readonly Dictionary<(string, string), CustomStringTokenResolver> _tokenResolvers = new()
    {
        { ("configuration", "name"), (c) => c.Configuration?.ConfigName },
        { ("platform", "name"), (c) => c.Platform?.PlatformName },
        { ("platform", "system"), (c) => c.Platform is not null ? KdlEnumUtils.GetName(c.Platform.System) : null },
        { ("platform", "arch"), (c) => c.Platform is not null ? KdlEnumUtils.GetName(c.Platform.Arch) : null },
        { ("host", "name"), (c) => KdlEnumUtils.GetName(c.Host) },
    };

    private readonly Dictionary<string, CustomStringTokenTransformer> _tokenTransformers = new()
    {
        { "lower", (v) => v.ToLower() },
        { "upper", (v) => v.ToUpper() },
        { "trim", (v) => v.Trim() },
        { "dirname", (v) => Path.GetDirectoryName(v) },
        { "basename", (v) => Path.GetFileName(v) },
        { "extname", (v) => Path.GetExtension(v)?.TrimStart('.') },
        { "extension", (v) => Path.GetExtension(v) },
        { "noextension", (v) => Path.ChangeExtension(v, null) },
    };

    private readonly Dictionary<string, KdlDocument> _kdlFiles = [];

    private readonly SortedDictionary<string, ConfigurationNode> _configurations = [];
    private readonly SortedDictionary<string, PlatformNode> _platforms = [];
    private readonly SortedDictionary<string, object?> _optionValues = [];
    private readonly SortedDictionary<ProjectBuildId, ResolvedProjectTree> _resolvedTrees = [];

    private readonly List<ProjectOption> _projectOptions = [];
    private readonly List<(KdlNode RawNode, INode Scope)> _deferredNodes = [];

    private KdlDocument? _projectDocument;

    public string ProjectPath { get; private set; } = string.Empty;
    public KdlDocument ProjectDocument => _projectDocument ?? throw new Exception("Project not loaded. Call LoadProject() first.");
    public IReadOnlyList<ProjectOption> ProjectOptions => _projectOptions;
    public IReadOnlyDictionary<string, object?> ProjectOptionValues => _optionValues;

    public IReadOnlyDictionary<(string, string), CustomStringTokenResolver> TokenResolvers => _tokenResolvers;
    public IReadOnlyDictionary<string, CustomStringTokenTransformer> TokenTransformers => _tokenTransformers;

    public void RegisterNode<T>(bool overwrite = false) where T : class, INode
    {
        string name = T.NodeTraits.Name;
        if (overwrite)
        {
            _nodeTypes[name] = typeof(T);
            _nodeTraits[name] = T.NodeTraits;
        }
        else if (_nodeTypes.TryAdd(name, typeof(T)))
        {
            _nodeTraits[name] = T.NodeTraits;
        }
        else
        {
            throw new Exception($"A node type for '{name}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void RegisterNodeGenerator<T>(bool overwrite = false) where T : class, INodeGenerator
    {
        string name = T.GeneratorTraits.Name;
        if (overwrite)
        {
            _nodeGenerators[name] = typeof(T);
        }
        else if (!_nodeGenerators.TryAdd(name, typeof(T)))
        {
            throw new Exception($"A node generator for '{name}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void RegisterTokenResolver(string contextName, string propertyName, CustomStringTokenResolver resolver, bool overwrite = false)
    {
        if (overwrite)
        {
            _tokenResolvers[(contextName, propertyName)] = resolver;
        }
        else if (!_tokenResolvers.TryAdd((contextName, propertyName), resolver))
        {
            throw new Exception($"A token resolver for context '{contextName}' and property '{propertyName}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void RegisterTokenTransformer(string name, CustomStringTokenTransformer transformer, bool overwrite = false)
    {
        if (overwrite)
        {
            _tokenTransformers[name] = transformer;
        }
        else if (!_tokenTransformers.TryAdd(name, transformer))
        {
            throw new Exception($"A token transformer for name '{name}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void LoadProject(string projectPath)
    {
        if (!Path.IsPathRooted(projectPath))
        {
            projectPath = Path.GetFullPath(projectPath);
        }

        ProjectPath = projectPath;

        _projectDocument = LoadFile(projectPath);

        if (_projectDocument.Nodes.Count != 1 || _projectDocument.Nodes[0].Name != ProjectNode.NodeTraits.Name)
        {
            throw new Exception($"Project invalid. Expected a single root '{ProjectNode.NodeTraits.Name}' node in file: {projectPath}");
        }

        ResolveImports(_projectDocument);

        _configurations.Clear();
        _platforms.Clear();
        _projectOptions.Clear();
        foreach (KdlNode node in _projectDocument.GetAllNodes())
        {
            if (node.Name == OptionNode.NodeTraits.Name)
            {
                OptionNode option = new(node);
                _projectOptions.Add(new ProjectOption(option, option.OptionType switch
                {
                    EOptionType.Bool => new Option<bool?>("--" + option.OptionName, () => option.GetDefaultBool(), option.HelpText),
                    EOptionType.Int => new Option<long?>("--" + option.OptionName, () => option.GetDefaultNumber<long>(), option.HelpText),
                    EOptionType.UInt => new Option<ulong?>("--" + option.OptionName, () => option.GetDefaultNumber<ulong>(), option.HelpText),
                    EOptionType.Float => new Option<double?>("--" + option.OptionName, () => option.GetDefaultNumber<double>(), option.HelpText),
                    EOptionType.String => new Option<string?>("--" + option.OptionName, () => option.GetDefaultString(), option.HelpText),
                    _ => throw new Exception($"Unknown option type: {option.OptionType}")
                }));
            }
            else if (node.Name == ConfigurationNode.NodeTraits.Name)
            {
                ConfigurationNode configuration = new(node);
                if (!_configurations.TryAdd(configuration.ConfigName, configuration))
                {
                    throw new NodeParseException(node, $"Encountered duplicate configuration name '{configuration.ConfigName}'. Configuration names must be unique. Previously defined in: {_configurations[configuration.ConfigName].Node.SourceInfo.ToErrorString()}");
                }
            }
            else if (node.Name == PlatformNode.NodeTraits.Name)
            {
                PlatformNode platform = new(node);
                if (!_platforms.TryAdd(platform.PlatformName, platform))
                {
                    throw new NodeParseException(node, $"Encountered duplicate platform name '{platform.PlatformName}'. Platform names must be unique. Previously defined in: {_platforms[platform.PlatformName].Node.SourceInfo.ToErrorString()}");
                }
            }
        }
    }

    public void ParseProject(InvocationContext invocationContext)
    {
        // TODO: For parse project:
        // 1. Setup project options
        // 2. Collect the build matrix (configurations x platforms)
        // 3. Create a ResolvedProjectTree for each configuration/platform combo
        // 4. Validate the project tree

        // TODO: Should I add a mechanism for disallowing certain node types from being in a WhenNode?
        // For example, when { module {} } doesn't make sense.

        if (_projectDocument is null)
        {
            throw new Exception("Project not loaded. Call LoadProject() first.");
        }

        // Collect the values for the project options
        foreach (ProjectOption projectOption in _projectOptions)
        {
            object? value = invocationContext.ParseResult.GetValueForOption(projectOption.Option);
            if (value is null && projectOption.Node.EnvVarName is not null)
            {
                value = Environment.GetEnvironmentVariable(projectOption.Node.EnvVarName);
            }

            if (value is not null)
            {
                _optionValues.Add(projectOption.Node.OptionName, value);
            }
        }

        // Create a resolved project tree for each configuration/platform combo
        foreach ((string configurationName, ConfigurationNode configuration) in _configurations)
        {
            foreach ((string platformName, PlatformNode platform) in _platforms)
            {
                ProjectBuildId buildId = new(configurationName, platformName);
                ResolvedProjectTree resolvedTree = new(this, configuration, platform, _projectDocument.Nodes[0]);
                _resolvedTrees.Add(buildId, resolvedTree);
            }
        }
    }

    public INodeTraits GetNodeTraits(KdlNode node)
    {
        if (_nodeTraits.TryGetValue(node.Name, out INodeTraits? traits))
        {
            return traits;
        }

        throw new NodeParseException(node, $"'{node.Name}' is not a recognized node type. Was it registered?");
    }

    public T CreateSemanticNode<T>(KdlNode node) where T : class, INode
    {
        return CreateSemanticNode(node) as T
            ?? throw new NodeParseException(node, $"Failed to create semantic node of type '{typeof(T)}' for KDL node '{node.Name}'.");
    }

    public INode CreateSemanticNode(KdlNode node)
    {
        if (node.Parent is KdlNode scope)
        {
            INodeTraits scopeTraits = GetNodeTraits(scope);
            if (scopeTraits.ChildNodeType is not null)
            {
                return Activator.CreateInstance(scopeTraits.ChildNodeType, node) as INode
                    ?? throw new NodeParseException(node, $"Activator.CreateInstance failed to allocate node instance for type '{scopeTraits.ChildNodeType}'.");
            }
        }

        if (_nodeTypes.TryGetValue(node.Name, out Type? nodeType))
        {
            return Activator.CreateInstance(nodeType, node) as INode
                ?? throw new NodeParseException(node, $"Activator.CreateInstance failed to allocate node instance for type '{nodeType}'.");
        }

        throw new NodeParseException(node, $"'{node.Name}' is not a recognized node type. Was it registered?");
    }

    public INodeGenerator CreateGeneratorForNode(KdlNode generatorNode)
    {
        Debug.Assert(generatorNode.Name.StartsWith(':'));

        string generatorName = generatorNode.Name[1..];
        if (!_nodeGenerators.TryGetValue(generatorName, out Type? generatorType))
        {
            throw new NodeParseException(generatorNode, $"Unknown generator type '{generatorName}'.");
        }

        return Activator.CreateInstance(generatorType, this) as INodeGenerator
            ?? throw new NodeParseException(generatorNode, $"Activator.CreateInstance failed to allocate node generator instance for type '{generatorType}'.");
    }

    public List<ConfigurationNode> GetDefaultConfigurations()
    {
        KdlSourceInfo baseSource = new(ProjectPath, 0, 0);

        KdlNode debugNode = new("configuration") { SourceInfo = baseSource };
        debugNode.Arguments.Add(new KdlString("Debug") { SourceInfo = baseSource });

        KdlNode releaseNode = new("configuration") { SourceInfo = baseSource };
        releaseNode.Arguments.Add(new KdlString("Release") { SourceInfo = baseSource });

        return
        [
            new ConfigurationNode(debugNode),
            new ConfigurationNode(releaseNode),
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

        string suffix = RuntimeInformation.OSArchitecture switch
        {
            Architecture.X86 => "32",
            Architecture.X64 => "64",
            Architecture.Arm => "Arm32",
            Architecture.Arm64 => "Arm64",
            _ => throw new Exception("Unsupported architecture.")
        };

        KdlSourceInfo baseSource = new(ProjectPath, 0, 0);

        KdlNode node = new("platform") { SourceInfo = baseSource };
        node.Properties.Add("arch", new KdlString(arch) { SourceInfo = baseSource });

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            node.Arguments.Add(new KdlString("Win" + suffix) { SourceInfo = baseSource });
            node.Properties.Add("system", new KdlString("windows") { SourceInfo = baseSource });
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            node.Arguments.Add(new KdlString("Linux" + suffix) { SourceInfo = baseSource });
            node.Properties.Add("system", new KdlString("linux") { SourceInfo = baseSource });
        }
        else
        {
            throw new Exception("Unsupported host platform.");
        }

        return [new PlatformNode(node)];
    }

    private KdlDocument LoadFile(string path)
    {
        if (!Path.IsPathRooted(path))
        {
            throw new ArgumentException($"Path must be absolute: {path}", nameof(path));
        }

        if (_kdlFiles.TryGetValue(path, out KdlDocument? document))
        {
            return document;
        }

        KdlDocument doc = KdlDocument.FromFile(path);
        _kdlFiles[path] = doc;

        return doc;
    }

    private void ResolveImports(KdlDocument document)
    {
        List<KdlNode> importNodes = [.. document.GetNodesByName("import")];
        ResolveImports(importNodes);
    }

    private void ResolveImports(List<KdlNode> importNodes)
    {
        foreach (KdlNode importNode in importNodes)
        {
            if (importNode.Parent is null || !ImportNode.NodeTraits.ValidScopes.Contains(importNode.Parent.Name))
            {
                throw new NodeParseException(importNode, $"Invalid parent scope. Expected one of: {string.Join(", ", ImportNode.NodeTraits.ValidScopes)}.");
            }

            if (!importNode.TryGetArgumentValue(0, out string? importPath) || string.IsNullOrEmpty(importPath))
            {
                throw new NodeParseException(importNode, "Expected exactly one string argument for the import path.");
            }

            if (!Path.GetExtension(importPath).Equals(".kdl", StringComparison.OrdinalIgnoreCase))
            {
                importPath = Path.Combine(importPath, "he_plugin.kdl");
            }

            string directory = Path.GetDirectoryName(importNode.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();
            Matcher matcher = new();
            matcher.AddInclude(importPath);
            foreach (string importMatchedPath in matcher.GetResultsInFullPath(directory))
            {
                KdlDocument importedDoc = LoadFile(importMatchedPath);
                importNode.ReplaceInParent(importedDoc.Nodes);
                ResolveImports(importedDoc);
            }
        }
    }
}
