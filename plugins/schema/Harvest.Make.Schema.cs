// Copyright Chad Engler

using Harvest.Common;
using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Extensions.Common;
using Harvest.Make.Projects;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;

namespace Harvest.Make.Schema;


public sealed class SchemaCompileExtension : IHarvestMakeExtension
{
    public void ConfigureServices(IServiceCollection services, ILogger logger)
    {
    }

    public void Startup(IServiceProvider services)
    {
        IProjectService projectService = services.GetRequiredService<IProjectService>();
        projectService.RegisterNode<SchemaCompileNode>();
    }

    public void Shutdown()
    {
    }
}

public sealed class SchemaCompileNodeTraits : NodeBaseTraits
{
    public override string Name => "schema_compile";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "scope", NodeValueDef_String.Optional("public") },
        { "group", NodeValueDef_String.Optional() },
    };

    protected override void ValidateArguments(KdlNode node)
    {
        if (node.Arguments.Count == 0)
        {
            throw new NodeParseException(node, $"'{node.Name}' nodes must specify at least one target argument.");
        }

        for (int i = 0; i < node.Arguments.Count; ++i)
        {
            if (node.Arguments[i] is not KdlString)
            {
                throw new NodeParseException(node, $"Arguments of '{node.Name}' nodes must be strings.");
            }
        }
    }

    public override void Validate(KdlNode node)
    {
        base.Validate(node);

        bool hasFiles = false;
        bool hasIncludeDirs = false;
        foreach (KdlNode child in node.Children)
        {
            switch (child.Name)
            {
                case "files":
                    hasFiles = true;
                    break;
                case "include_dirs":
                    hasIncludeDirs = true;
                    break;
                case "dependencies":
                    break;
                default:
                    throw new NodeParseException(child, $"'{node.Name}' nodes only support 'files', 'include_dirs', and 'dependencies' children.");
            }
        }

        if (!hasFiles)
        {
            throw new NodeParseException(node, $"'{node.Name}' nodes require a 'files' child.");
        }

        if (!hasIncludeDirs)
        {
            throw new NodeParseException(node, $"'{node.Name}' nodes require an 'include_dirs' child.");
        }
    }

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        Validate(source);
        KdlNode resolvedSource = CreateResolvedSchemaNode(source, replacer, resolver);
        SchemaCompileNode schemaCompile = new(resolvedSource);
        ModuleNode ownerModule = new(target);
        KdlNode ownerSourceModule = ExtensionNodeUtils.GetOwningModule(source);
        KdlNode pluginTarget = resolver.ProjectContext.Plugin?.Node ?? ExtensionNodeUtils.GetOwningPlugin(target);

        string generatedModuleName = $"{ownerModule.ModuleName}__schemac_{ExtensionNodeUtils.GetLexicalIndex(ownerSourceModule, source, Name)}";
        FilesNode filesNode = ExtensionNodeUtils.GetRequiredNode<FilesNode>(resolvedSource, FilesNode.NodeTraits.Name);
        IncludeDirsNode includeDirsNode = ExtensionNodeUtils.GetRequiredNode<IncludeDirsNode>(resolvedSource, IncludeDirsNode.NodeTraits.Name);
        DependenciesNode? dependenciesNode = ExtensionNodeUtils.GetOptionalNode<DependenciesNode>(resolvedSource, DependenciesNode.NodeTraits.Name);

        KdlNode generatedModule = ExtensionNodeUtils.CreateNode(ModuleNode.NodeTraits.Name, source.SourceInfo,
            new KdlString(generatedModuleName),
            properties: new Dictionary<string, KdlValue>()
            {
                ["kind"] = new KdlString("lib_static"),
                ["group"] = new KdlString(schemaCompile.Group ?? "_generated/schema"),
            });

        KdlNode generatedFilesNode = ExtensionNodeUtils.CreateNode(FilesNode.NodeTraits.Name, source.SourceInfo);
        generatedModule.AddChild(generatedFilesNode);

        IReadOnlyList<string> includeRoots = [.. includeDirsNode.Entries.Select((entry) => entry.Path)];
        IReadOnlyList<string> dependencyModuleNames = dependenciesNode?.Entries.Select((entry) => entry.DependencyName).Distinct(StringComparer.Ordinal).ToList() ?? [];

        foreach (FilesEntryNode fileEntry in filesNode.Entries)
        {
            string sourceFilePath = fileEntry.FilePath;
            string moduleRelativePath = ExtensionNodeUtils.GetModuleRelativePath(ownerSourceModule, sourceFilePath);
            string ruleName = $"{ownerModule.ModuleName}_{ExtensionNodeUtils.SanitizeIdentifier(Path.GetFileNameWithoutExtension(sourceFilePath))}_schemac";
            string outputDirToken = GetSchemaOutputDirToken(ownerModule.ModuleName, sourceFilePath, includeRoots);

            generatedFilesNode.AddChild(ExtensionNodeUtils.CreateNode(moduleRelativePath, source.SourceInfo,
                properties: new Dictionary<string, KdlValue>()
                {
                    ["action"] = new KdlString("build"),
                    ["build_rule"] = new KdlString(ruleName),
                }));

            string command = BuildSchemaCommand(schemaCompile, outputDirToken, includeRoots, dependencyModuleNames, resolver.ProjectContext.ProjectService);
            generatedModule.AddChild(ExtensionNodeUtils.CreateBuildRuleNode(
                source.SourceInfo,
                ruleName,
                "Compiling schema file %(FullPath)",
                [$"echo {command}", command],
                ["${module[he_schemac].build_target}"],
                [
                    $"{outputDirToken}/{Path.GetFileName(sourceFilePath)}.h",
                    $"{outputDirToken}/{Path.GetFileName(sourceFilePath)}.cpp",
                ],
                linkOutput: true));
        }

        List<KdlNode> dependencyEntries =
        [
            ExtensionNodeUtils.CreateDependencyEntry("he_core", source.SourceInfo),
        ];
        dependencyEntries.AddRange(CreateSchemaDependencyEntries(source.SourceInfo, dependencyModuleNames, resolver.ProjectContext.ProjectService));
        dependencyEntries.Add(ExtensionNodeUtils.CreateDependencyEntry("he_schema", source.SourceInfo));
        dependencyEntries.Add(ExtensionNodeUtils.CreateDependencyEntry("he_schemac", source.SourceInfo, kind: "order"));
        generatedModule.AddChild(ExtensionNodeUtils.CreateDependenciesNode(source.SourceInfo, dependencyEntries));

        KdlNode publicNode = ExtensionNodeUtils.CreateNode(PublicNode.NodeTraits.Name, source.SourceInfo);
        publicNode.AddChild(ExtensionNodeUtils.CreateStringEntriesNode(IncludeDirsNode.NodeTraits.Name, source.SourceInfo, [$"${{module[{ownerModule.ModuleName}].gen_dir}}/include"]));
        generatedModule.AddChild(publicNode);

        pluginTarget.AddChild(resolver.CreateResolvedNode(generatedModule, includeChildren: true));
        ExtensionNodeUtils.AddOwnerDependency(target, source.SourceInfo, generatedModuleName, schemaCompile.IsPublicScope, resolver);
        ExtensionNodeUtils.AddOwnerDependency(target, source.SourceInfo, generatedModuleName, schemaCompile.IsPublicScope, resolver, kind: "order");

        resolvedNode = null;
        return true;
    }

    public override INode CreateNode(KdlNode node) => new SchemaCompileNode(node);

    private static KdlNode CreateResolvedSchemaNode(KdlNode source, StringTokenReplacer replacer, NodeResolver resolver)
    {
        KdlNode resolvedNode = new("schema_compile", source.Type)
        {
            SourceInfo = source.SourceInfo,
        };

        NodeResolver.ResolveDefaultNodeProperties(resolvedNode, new SchemaCompileNodeTraits(), replacer);
        NodeResolver.ResolveNodeProperties(resolvedNode, source, new SchemaCompileNodeTraits(), replacer);

        foreach (KdlValue argument in source.Arguments)
        {
            if (argument is not KdlString argumentString)
            {
                throw new NodeParseException(source, $"Arguments of '{source.Name}' nodes must be strings.");
            }

            resolvedNode.Arguments.Add(new KdlString(replacer.ReplaceTokens(argumentString.Value), argumentString.Type)
            {
                SourceInfo = argument.SourceInfo,
            });
        }

        foreach (KdlNode child in source.Children)
        {
            INodeTraits childTraits = resolver.ProjectContext.ProjectService.GetNodeTraits(child);
            if (!childTraits.TryResolveChild(resolvedNode, child, replacer, resolver, out KdlNode? resolvedChild))
            {
                resolvedChild = resolver.CreateResolvedNode(child, includeChildren: true);
            }

            if (resolvedChild is not null)
            {
                resolvedNode.AddChild(resolvedChild);
            }
        }

        return resolvedNode;
    }

    private static IEnumerable<KdlNode> CreateSchemaDependencyEntries(KdlSourceInfo sourceInfo, IEnumerable<string> dependencyModuleNames, IProjectService projectService)
    {
        foreach (string dependencyModuleName in dependencyModuleNames.Distinct(StringComparer.Ordinal))
        {
            KdlNode sourceModule = ExtensionNodeUtils.GetSourceModule(projectService, dependencyModuleName);
            int schemaCompileCount = sourceModule.GetAllDescendants().Count((node) => node.Name == "schema_compile");
            if (schemaCompileCount == 0)
            {
                throw new NodeParseException(sourceModule, $"Module '{dependencyModuleName}' was referenced by schema_compile dependencies, but it does not define any schema_compile nodes.");
            }

            for (int i = 1; i <= schemaCompileCount; ++i)
            {
                yield return ExtensionNodeUtils.CreateDependencyEntry($"{dependencyModuleName}__schemac_{i}", sourceInfo);
            }
        }
    }

    private static string BuildSchemaCommand(SchemaCompileNode schemaCompile, string outputDirToken, IEnumerable<string> includeRoots, IEnumerable<string> dependencyModuleNames, IProjectService projectService)
    {
        List<string> args = ["${module[he_schemac].build_target}"];

        foreach (string target in schemaCompile.Targets)
        {
            args.Add("-t");
            args.Add(target);
        }

        foreach (string includeDir in includeRoots)
        {
            args.Add("-I");
            args.Add($"\"{includeDir}\"");
        }

        foreach (string includeDir in CollectSchemaDependencyIncludeDirs(projectService, dependencyModuleNames))
        {
            args.Add("-I");
            args.Add($"\"{includeDir}\"");
        }

        foreach (string includeDir in GetModulePublicIncludeDirs(projectService, "he_schema"))
        {
            args.Add("-I");
            args.Add($"\"{includeDir}\"");
        }

        args.Add("-o");
        args.Add($"\"{outputDirToken}\"");
        args.Add("\"%(FullPath)\"");

        return string.Join(' ', args);
    }

    private static IEnumerable<string> CollectSchemaDependencyIncludeDirs(IProjectService projectService, IEnumerable<string> dependencyModuleNames)
    {
        HashSet<string> visitedModules = [];
        HashSet<string> includeDirs = [];

        foreach (string dependencyModuleName in dependencyModuleNames)
        {
            CollectSchemaDependencyIncludeDirs(projectService, dependencyModuleName, visitedModules, includeDirs);
        }

        return includeDirs.Order(StringComparer.Ordinal);
    }

    private static void CollectSchemaDependencyIncludeDirs(IProjectService projectService, string dependencyModuleName, HashSet<string> visitedModules, HashSet<string> includeDirs)
    {
        if (!visitedModules.Add(dependencyModuleName))
        {
            return;
        }

        KdlNode sourceModule = ExtensionNodeUtils.GetSourceModule(projectService, dependencyModuleName);
        bool hasSchemaCompile = false;
        string moduleDir = Path.GetDirectoryName(sourceModule.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();
        foreach (KdlNode schemaNode in sourceModule.GetAllDescendants().Where((node) => node.Name == "schema_compile"))
        {
            hasSchemaCompile = true;

            KdlNode? includeDirsNode = schemaNode.Children.FirstOrDefault((child) => child.Name == IncludeDirsNode.NodeTraits.Name);
            if (includeDirsNode is not null)
            {
                foreach (KdlNode includeEntry in includeDirsNode.Children)
                {
                    includeDirs.Add(Path.GetFullPath(includeEntry.Name, moduleDir));
                }
            }

            KdlNode? dependenciesNode = schemaNode.Children.FirstOrDefault((child) => child.Name == DependenciesNode.NodeTraits.Name);
            if (dependenciesNode is not null)
            {
                foreach (KdlNode dependencyEntry in dependenciesNode.Children)
                {
                    CollectSchemaDependencyIncludeDirs(projectService, dependencyEntry.Name, visitedModules, includeDirs);
                }
            }
        }

        if (!hasSchemaCompile)
        {
            throw new NodeParseException(sourceModule, $"Module '{dependencyModuleName}' was referenced by schema_compile dependencies, but it does not define any schema_compile nodes.");
        }
    }

    private static IEnumerable<string> GetModulePublicIncludeDirs(IProjectService projectService, string moduleName)
    {
        KdlNode sourceModule = ExtensionNodeUtils.GetSourceModule(projectService, moduleName);
        string moduleDir = Path.GetDirectoryName(sourceModule.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();

        foreach (KdlNode publicNode in sourceModule.Children.Where((child) => child.Name == PublicNode.NodeTraits.Name))
        {
            foreach (KdlNode includeDirsNode in publicNode.Children.Where((child) => child.Name == IncludeDirsNode.NodeTraits.Name))
            {
                foreach (KdlNode includeEntry in includeDirsNode.Children)
                {
                    yield return Path.GetFullPath(includeEntry.Name, moduleDir);
                }
            }
        }
    }

    private static string GetSchemaOutputDirToken(string ownerModuleName, string filePath, IEnumerable<string> includeRoots)
    {
        if (!ExtensionNodeUtils.TryGetContainingRoot(filePath, includeRoots, out _, out string relativePath))
        {
            throw new NodeParseException(new KdlNode(filePath), $"Schema file '{filePath}' is not contained by any schema_compile include_dirs entry.");
        }

        string? relativeDir = Path.GetDirectoryName(relativePath)?.Replace('\\', '/');
        return string.IsNullOrEmpty(relativeDir)
            ? $"${{module[{ownerModuleName}].gen_dir}}/include"
            : $"${{module[{ownerModuleName}].gen_dir}}/include/{relativeDir}";
    }
}

public sealed class SchemaCompileNode(KdlNode node) : NodeBase<SchemaCompileNodeTraits>(node)
{
    public IReadOnlyList<string> Targets => [.. Node.Arguments.Select((arg) => arg.GetValueString())];
    public string Scope => GetValue<string>("scope");
    public string? Group => TryGetValue("group", out string? group) ? group : null;
    public bool IsPublicScope => string.Equals(Scope, "public", StringComparison.Ordinal);
}
