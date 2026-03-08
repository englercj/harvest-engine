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

namespace Harvest.Make.Bin2C;


public sealed class Bin2CCompileExtension : IHarvestMakeExtension
{
    public void ConfigureServices(IServiceCollection services, ILogger logger)
    {
    }

    public void Startup(IServiceProvider services)
    {
        IProjectService projectService = services.GetRequiredService<IProjectService>();
        projectService.RegisterNode<Bin2CCompileNode>();
    }

    public void Shutdown()
    {
    }
}

public sealed class Bin2CCompileNodeTraits : NodeBaseTraits
{
    public override string Name => "bin2c_compile";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "scope", NodeValueDef_String.Optional("public") },
        { "group", NodeValueDef_String.Optional() },
        { "text", NodeValueDef_Bool.Optional(false) },
        { "compress", NodeValueDef_Bool.Optional(false) },
    };

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
                    break;
                default:
                    throw new NodeParseException(child, $"'{node.Name}' nodes only support 'files' and 'include_dirs' children.");
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

        KdlNode resolvedSource = resolver.CreateResolvedNode(source, includeChildren: true);
        Bin2CCompileNode bin2cCompile = new(resolvedSource);

        ModuleNode ownerModule = new(target);
        KdlNode ownerSourceModule = ExtensionNodeUtils.GetOwningModule(source);
        KdlNode pluginTarget = resolver.ProjectContext.Plugin?.Node ?? ExtensionNodeUtils.GetOwningPlugin(target);

        string generatedModuleName = $"{ownerModule.ModuleName}__bin2c_{ExtensionNodeUtils.GetLexicalIndex(ownerSourceModule, source, Name)}";
        FilesNode filesNode = ExtensionNodeUtils.GetRequiredNode<FilesNode>(resolvedSource, FilesNode.NodeTraits.Name);
        IncludeDirsNode? includeDirsNode = ExtensionNodeUtils.GetOptionalNode<IncludeDirsNode>(resolvedSource, IncludeDirsNode.NodeTraits.Name);
        IReadOnlyList<string> includeRoots = includeDirsNode?.Entries.Select((entry) => entry.Path).ToList() ?? [];

        KdlNode generatedModule = ExtensionNodeUtils.CreateNode(ModuleNode.NodeTraits.Name, source.SourceInfo,
            new KdlString(generatedModuleName),
            properties: new Dictionary<string, KdlValue>()
            {
                ["kind"] = new KdlString("custom"),
                ["group"] = new KdlString(bin2cCompile.Group ?? "_generated/bin2c"),
            });

        KdlNode generatedFilesNode = ExtensionNodeUtils.CreateNode(FilesNode.NodeTraits.Name, source.SourceInfo);
        generatedModule.AddChild(generatedFilesNode);

        foreach (FilesEntryNode fileEntry in filesNode.Entries)
        {
            string sourceFilePath = fileEntry.FilePath;
            string moduleRelativePath = ExtensionNodeUtils.GetModuleRelativePath(ownerSourceModule, sourceFilePath);
            string ruleName = $"{ownerModule.ModuleName}_{ExtensionNodeUtils.SanitizeIdentifier(Path.GetFileNameWithoutExtension(sourceFilePath))}_bin2c";
            string outputFileToken = GetOutputFileToken(ownerModule.ModuleName, ownerSourceModule, sourceFilePath, includeRoots);
            string variableName = $"c_{ExtensionNodeUtils.SanitizeIdentifier(Path.GetFileName(sourceFilePath))}";

            generatedFilesNode.AddChild(ExtensionNodeUtils.CreateNode(moduleRelativePath, source.SourceInfo,
                properties: new Dictionary<string, KdlValue>()
                {
                    ["action"] = new KdlString("build"),
                    ["build_rule"] = new KdlString(ruleName),
                }));

            string command = BuildCommand(bin2cCompile, variableName, outputFileToken);
            generatedModule.AddChild(ExtensionNodeUtils.CreateBuildRuleNode(
                source.SourceInfo,
                ruleName,
                "Creating C header for file %(FullPath)",
                [$"echo {command}", command],
                ["${module[he_bin2c].build_target}"],
                [outputFileToken],
                linkOutput: false));
        }

        generatedModule.AddChild(ExtensionNodeUtils.CreateDependenciesNode(source.SourceInfo,
        [
            ExtensionNodeUtils.CreateDependencyEntry("he_bin2c", source.SourceInfo, kind: "order"),
        ]));

        if (includeRoots.Count > 0)
        {
            KdlNode publicNode = ExtensionNodeUtils.CreateNode(PublicNode.NodeTraits.Name, source.SourceInfo);
            publicNode.AddChild(ExtensionNodeUtils.CreateStringEntriesNode(IncludeDirsNode.NodeTraits.Name, source.SourceInfo,
                includeRoots.Select((includeRoot) => ExtensionNodeUtils.GetGeneratedDirToken(ownerModule.ModuleName, ExtensionNodeUtils.GetModuleRelativePath(ownerSourceModule, includeRoot)))));
            generatedModule.AddChild(publicNode);
        }

        pluginTarget.AddChild(resolver.CreateResolvedNode(generatedModule, includeChildren: true));
        ExtensionNodeUtils.AddOwnerDependency(target, source.SourceInfo, generatedModuleName, bin2cCompile.IsPublicScope, resolver);

        resolvedNode = null;
        return true;
    }

    public override INode CreateNode(KdlNode node) => new Bin2CCompileNode(node);

    private static string BuildCommand(Bin2CCompileNode node, string variableName, string outputFileToken)
    {
        List<string> args = ["${module[he_bin2c].build_target}"];
        if (node.Text)
        {
            args.Add("-t");
        }

        if (node.Compress)
        {
            args.Add("-c");
        }

        args.Add("-n");
        args.Add(variableName);
        args.Add("-f");
        args.Add("\"%(FullPath)\"");
        args.Add("-o");
        args.Add($"\"{outputFileToken}\"");
        return string.Join(' ', args);
    }

    private static string GetOutputFileToken(string ownerModuleName, KdlNode ownerSourceModule, string sourceFilePath, IReadOnlyList<string> includeRoots)
    {
        string outputDirToken = GetOutputDirToken(ownerModuleName, ownerSourceModule, sourceFilePath, includeRoots);
        return $"{outputDirToken}/{Path.GetFileName(sourceFilePath)}.h";
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

public sealed class Bin2CCompileNode(KdlNode node) : NodeBase<Bin2CCompileNodeTraits>(node)
{
    public string Scope => GetValue<string>("scope");
    public string? Group => TryGetValue("group", out string? group) ? group : null;
    public bool Text => GetValue<bool>("text");
    public bool Compress => GetValue<bool>("compress");
    public bool IsPublicScope => string.Equals(Scope, "public", StringComparison.Ordinal);
}
