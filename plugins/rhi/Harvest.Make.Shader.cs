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

namespace Harvest.Make.Shader;


public sealed class ShaderCompileExtension : IHarvestMakeExtension
{
    public void ConfigureServices(IServiceCollection services, ILogger logger)
    {
    }

    public void Startup(IServiceProvider services)
    {
        IProjectService projectService = services.GetRequiredService<IProjectService>();
        projectService.RegisterNode<ShaderCompileNode>();
    }

    public void Shutdown()
    {
    }
}

public sealed class ShaderCompileNodeTraits : NodeBaseTraits
{
    public override string Name => "shader_compile";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "scope", NodeValueDef_String.Optional("public") },
        { "group", NodeValueDef_String.Optional() },
        { "optimize", NodeValueDef_String.Optional() },
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
        foreach (KdlNode child in node.Children)
        {
            switch (child.Name)
            {
                case "files":
                    hasFiles = true;
                    break;
                case "include_dirs":
                case "defines":
                    break;
                default:
                    throw new NodeParseException(child, $"'{node.Name}' nodes only support 'files', 'include_dirs', and 'defines' children.");
            }
        }

        if (!hasFiles)
        {
            throw new NodeParseException(node, $"'{node.Name}' nodes require a 'files' child.");
        }
    }

    public override bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        Validate(source);
        KdlNode resolvedSource = CreateResolvedShaderNode(source, replacer, resolver);
        ShaderCompileNode shaderCompile = new(resolvedSource);
        ModuleNode ownerModule = new(target);
        KdlNode ownerSourceModule = ExtensionNodeUtils.GetOwningModule(source);
        KdlNode pluginTarget = resolver.ProjectContext.Plugin?.Node ?? ExtensionNodeUtils.GetOwningPlugin(target);

        string generatedModuleName = $"{ownerModule.ModuleName}__shaderc_{ExtensionNodeUtils.GetLexicalIndex(ownerSourceModule, source, Name)}";
        FilesNode filesNode = ExtensionNodeUtils.GetRequiredNode<FilesNode>(resolvedSource, FilesNode.NodeTraits.Name);
        IncludeDirsNode? includeDirsNode = ExtensionNodeUtils.GetOptionalNode<IncludeDirsNode>(resolvedSource, IncludeDirsNode.NodeTraits.Name);
        DefinesNode? definesNode = ExtensionNodeUtils.GetOptionalNode<DefinesNode>(resolvedSource, DefinesNode.NodeTraits.Name);
        IReadOnlyList<string> includeRoots = includeDirsNode?.Entries.Select((entry) => entry.Path).ToList() ?? [];
        IReadOnlyList<string> defineNames = definesNode?.Entries.Select((entry) => entry.DefineName).ToList() ?? [];

        KdlNode generatedModule = ExtensionNodeUtils.CreateNode(ModuleNode.NodeTraits.Name, source.SourceInfo,
            new KdlString(generatedModuleName),
            properties: new Dictionary<string, KdlValue>()
            {
                ["kind"] = new KdlString("custom"),
                ["group"] = new KdlString(shaderCompile.Group ?? "_generated/shader"),
            });

        KdlNode generatedFilesNode = ExtensionNodeUtils.CreateNode(FilesNode.NodeTraits.Name, source.SourceInfo);
        generatedModule.AddChild(generatedFilesNode);

        foreach (FilesEntryNode fileEntry in filesNode.Entries)
        {
            string sourceFilePath = fileEntry.FilePath;
            string moduleRelativePath = ExtensionNodeUtils.GetModuleRelativePath(ownerSourceModule, sourceFilePath);
            string ruleName = $"{ownerModule.ModuleName}_{ExtensionNodeUtils.SanitizeIdentifier(Path.GetFileNameWithoutExtension(sourceFilePath))}_shaderc";
            string outputDirToken = GetOutputDirToken(ownerModule.ModuleName, ownerSourceModule, sourceFilePath, includeRoots);

            generatedFilesNode.AddChild(ExtensionNodeUtils.CreateNode(moduleRelativePath, source.SourceInfo,
                properties: new Dictionary<string, KdlValue>()
                {
                    ["action"] = new KdlString("build"),
                    ["build_rule"] = new KdlString(ruleName),
                }));

            string command = BuildCommand(shaderCompile, includeRoots, defineNames, outputDirToken);
            generatedModule.AddChild(ExtensionNodeUtils.CreateBuildRuleNode(
                source.SourceInfo,
                ruleName,
                "Compiling shader file %(FullPath)",
                [$"echo {command}", command],
                ["${module[he_shaderc].build_target}"],
                [$"{outputDirToken}/{Path.GetFileNameWithoutExtension(sourceFilePath)}.shaders.h"],
                linkOutput: false));
        }

        generatedModule.AddChild(ExtensionNodeUtils.CreateDependenciesNode(source.SourceInfo,
        [
            ExtensionNodeUtils.CreateDependencyEntry("he_shaderc", source.SourceInfo, kind: "order"),
        ]));

        if (includeRoots.Count > 0)
        {
            KdlNode publicNode = ExtensionNodeUtils.CreateNode(PublicNode.NodeTraits.Name, source.SourceInfo);
            publicNode.AddChild(ExtensionNodeUtils.CreateStringEntriesNode(IncludeDirsNode.NodeTraits.Name, source.SourceInfo,
                includeRoots.Select((includeRoot) => ExtensionNodeUtils.GetGeneratedDirToken(ownerModule.ModuleName, ExtensionNodeUtils.GetModuleRelativePath(ownerSourceModule, includeRoot)))));
            generatedModule.AddChild(publicNode);
        }

        pluginTarget.AddChild(resolver.CreateResolvedNode(generatedModule, includeChildren: true));
        ExtensionNodeUtils.AddOwnerDependency(target, source.SourceInfo, generatedModuleName, shaderCompile.IsPublicScope, resolver);
        ExtensionNodeUtils.AddOwnerDependency(target, source.SourceInfo, generatedModuleName, shaderCompile.IsPublicScope, resolver, kind: "order");

        resolvedNode = null;
        return true;
    }

    public override INode CreateNode(KdlNode node) => new ShaderCompileNode(node);

    private static KdlNode CreateResolvedShaderNode(KdlNode source, StringTokenReplacer replacer, NodeResolver resolver)
    {
        KdlNode resolvedNode = new("shader_compile", source.Type)
        {
            SourceInfo = source.SourceInfo,
        };

        NodeResolver.ResolveDefaultNodeProperties(resolvedNode, new ShaderCompileNodeTraits(), replacer);
        NodeResolver.ResolveNodeProperties(resolvedNode, source, new ShaderCompileNodeTraits(), replacer);

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

    private static string BuildCommand(ShaderCompileNode node, IReadOnlyList<string> includeRoots, IReadOnlyList<string> defineNames, string outputDirToken)
    {
        List<string> args = ["${module[he_shaderc].build_target}"];

        foreach (string target in node.Targets)
        {
            args.Add("-t");
            args.Add(target);
        }

        foreach (string includeDir in includeRoots)
        {
            args.Add("-I");
            args.Add($"\"{includeDir}\"");
        }

        foreach (string defineName in defineNames)
        {
            args.Add("-D");
            args.Add(defineName);
        }

        if (!string.IsNullOrEmpty(node.Optimize))
        {
            args.Add("-O");
            args.Add(node.Optimize);
        }

        args.Add("-o");
        args.Add($"\"{outputDirToken}\"");
        args.Add("\"%(FullPath)\"");
        return string.Join(' ', args);
    }

    private static string GetOutputDirToken(string ownerModuleName, KdlNode ownerSourceModule, string sourceFilePath, IReadOnlyList<string> includeRoots)
    {
        if (includeRoots.Count > 0
            && ExtensionNodeUtils.TryGetContainingRoot(sourceFilePath, includeRoots, out string containingRoot, out string relativePath))
        {
            string rootRelativePath = ExtensionNodeUtils.GetModuleRelativePath(ownerSourceModule, containingRoot);
            string? relativeDir = Path.GetDirectoryName(relativePath)?.Replace('\\', '/');
            return string.IsNullOrEmpty(relativeDir)
                ? ExtensionNodeUtils.GetGeneratedDirToken(ownerModuleName, rootRelativePath)
                : ExtensionNodeUtils.GetGeneratedDirToken(ownerModuleName, $"{rootRelativePath}/{relativeDir}");
        }

        string moduleRelativePath = ExtensionNodeUtils.GetModuleRelativePath(ownerSourceModule, sourceFilePath);
        string? moduleRelativeDir = Path.GetDirectoryName(moduleRelativePath)?.Replace('\\', '/');
        return ExtensionNodeUtils.GetGeneratedDirToken(ownerModuleName, moduleRelativeDir ?? string.Empty);
    }
}

public sealed class ShaderCompileNode(KdlNode node) : NodeBase<ShaderCompileNodeTraits>(node)
{
    public IReadOnlyList<string> Targets => [.. Node.Arguments.Select((arg) => arg.GetValueString())];
    public string Scope => GetValue<string>("scope");
    public string? Group => TryGetValue("group", out string? group) ? group : null;
    public string? Optimize => TryGetValue("optimize", out string? optimize) ? optimize : null;
    public bool IsPublicScope => string.Equals(Scope, "public", StringComparison.Ordinal);
}
