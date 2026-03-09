// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;

namespace Harvest.Make.Projects.Tests;

public sealed class NodeCoverageTests(ProjectGenerationFixture fixture)
    : IClassFixture<ProjectGenerationFixture>
{
    [Fact]
    public void CoversAuthorsNode()
    {
        AuthorsNode authors = fixture.DebugTree.GetNodes<AuthorsNode>(fixture.PluginNode.Node).Single();
        AuthorsEntryNode author = authors.Entries.Single();
        Assert.Equal("Jane Doe", author.AuthorName);
        Assert.Equal("jane@example.com", author.AuthorEmail);
    }

    [Fact]
    public void CoversBuildEventNode()
    {
        BuildEventNode prebuild = fixture.DebugTree.GetMergedNode<BuildEventNode>(fixture.AppModule.Node, (n) => n.EventName == EBuildEvent.Prebuild, false);
        Assert.Equal("Prebuild Step", prebuild.EventMessage);
        Assert.Contains("PreBuildEvent", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversBuildOptionsNode()
    {
        BuildOptionsNode buildOptions = fixture.DebugTree.GetMergedNode<BuildOptionsNode>(fixture.AppModule.Node);
        Assert.Equal("pch.h", buildOptions.PchInclude);
        Assert.True(buildOptions.OpenMP);
        Assert.Contains("PrecompiledHeaderFile", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversBuildOutputNode()
    {
        BuildOutputNode buildOutput = fixture.DebugTree.GetMergedNode<BuildOutputNode>(fixture.AppModule.Node);
        Assert.StartsWith(fixture.RootDir, buildOutput.BinDir);
        Assert.EndsWith(Path.Combine(".build", "bin"), buildOutput.BinDir);
        Assert.Contains("OutDir", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversBuildRuleNode()
    {
        BuildRuleNode rule = fixture.DebugTree.GetMergedNode<BuildRuleNode>(fixture.AppModule.Node, (n) => n.RuleName == "gen_step", false);
        Assert.Equal("Run codegen", rule.Message);
        Assert.Contains("Run codegen", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversCodegenNode()
    {
        CodegenNode codegen = fixture.DebugTree.GetMergedNode<CodegenNode>(fixture.AppModule.Node);
        Assert.Equal(ECodegenMode.AVX2, codegen.CodegenMode);
        Assert.Contains("AdvancedVectorExtensions2", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversCommandNode()
    {
        BuildRuleNode rule = fixture.DebugTree.GetMergedNode<BuildRuleNode>(fixture.AppModule.Node, (n) => n.RuleName == "gen_step", false);
        CommandNode command = fixture.DebugTree.GetNodes<CommandNode>(rule.Node).First();
        Assert.Contains("mkdir", command.GetCommandString().ToLowerInvariant());
        Assert.Contains("Command", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversConfigurationNode()
    {
        Assert.Contains(new ProjectBuildId("Debug", "Win64"), fixture.ProjectService.ResolvedProjectTrees.Keys);
        Assert.Contains(new ProjectBuildId("Release", "Win64"), fixture.ProjectService.ResolvedProjectTrees.Keys);
        Assert.Contains("BuildType Name=\"Debug\"", fixture.SlnxText);
        Assert.Contains("BuildType Name=\"Release\"", fixture.SlnxText);
    }

    [Fact]
    public void CoversDefinesNode()
    {
        DefinesNode defines = fixture.DebugTree.GetMergedNode<DefinesNode>(fixture.AppModule.Node);
        Assert.Contains(defines.Entries, (entry) => entry.DefineName == "APP_DEF");
        Assert.Contains("APP_DEF", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversDependenciesNode()
    {
        List<DependenciesNode> appDependencies = [.. fixture.DebugTree.GetNodes<DependenciesNode>(fixture.AppModule.Node)];
        Assert.Contains(appDependencies.SelectMany((n) => n.Entries), (entry) => entry.DependencyName == "test_lib");

        List<DependenciesNode> toolDependencies = [.. fixture.DebugTree.GetNodes<DependenciesNode>(fixture.ToolModule.Node)];
        DependenciesEntryNode fileDependency = toolDependencies.SelectMany((n) => n.Entries).Single((entry) => entry.Kind == EDependencyKind.File);
        Assert.True(Path.IsPathRooted(fileDependency.DependencyName));
        Assert.EndsWith(Path.Combine("assets", "tool.txt"), fileDependency.DependencyName);

        Assert.Contains("ProjectReference Include=\"test_lib.vcxproj\"", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversDialectNode()
    {
        DialectNode dialect = fixture.DebugTree.GetMergedNode<DialectNode>(fixture.AppModule.Node);
        Assert.Equal(ECppDialect.Cpp20, dialect.CppDialect);
        Assert.Contains("LanguageStandard", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversExceptionsNode()
    {
        ExceptionsNode exceptions = fixture.DebugTree.GetMergedNode<ExceptionsNode>(fixture.AppModule.Node);
        Assert.Equal(EExceptionsMode.SEH, exceptions.ExceptionsMode);
        Assert.Contains("ExceptionHandling", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversExternalNode()
    {
        ExternalNode external = fixture.DebugTree.GetMergedNode<ExternalNode>(fixture.AppModule.Node);
        Assert.Equal(EWarningsLevel.Extra, external.WarningsLevel);
        Assert.Contains("ExternalWarningLevel", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversFetchNode()
    {
        InstallNode install = fixture.DebugTree.GetNodes<InstallNode>(fixture.PluginNode.Node).Single();
        FetchNode fetch = fixture.DebugTree.GetNodes<FetchNode>(install.Node).Single();
        Assert.Equal(EFetchArchiveFormat.Zip, fetch.ArchiveFormat);
        Assert.StartsWith("unit_dep-", fetch.ArchiveDirName, StringComparison.OrdinalIgnoreCase);
        Assert.Equal("dep-root", fetch.ArchiveBaseDir);
    }

    [Fact]
    public void CoversFetchNodeTarXzInference()
    {
        KdlNode node = new("fetch");
        node.Arguments.Add(KdlValue.From("archive"));
        node.Properties["url"] = KdlValue.From("https://example.com/archive.tar.xz");

        FetchNode fetch = new(node);
        Assert.Equal(EFetchArchiveFormat.TarXz, fetch.ArchiveFormat);
    }

    [Fact]
    public void CoversFilesNode()
    {
        FilesNode files = fixture.DebugTree.GetMergedNode<FilesNode>(fixture.AppModule.Node);
        Assert.Contains(files.Entries, (entry) => entry.FilePath.EndsWith("main.cpp", StringComparison.OrdinalIgnoreCase));
        Assert.Contains("ClCompile", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversFloatingPointNode()
    {
        FloatingPointNode floatingPoint = fixture.DebugTree.GetMergedNode<FloatingPointNode>(fixture.AppModule.Node);
        Assert.Equal(EFloatingPointMode.Strict, floatingPoint.Mode);
        Assert.Contains("FloatingPointModel", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversImportNode()
    {
        Assert.Equal(fixture.PluginFilePath, fixture.PluginNode.Node.SourceInfo.FilePath);
    }

    [Fact]
    public void CoversIncludeDirsNode()
    {
        IncludeDirsNode includeDirs = fixture.DebugTree.GetMergedNode<IncludeDirsNode>(fixture.AppModule.Node);
        Assert.Contains(includeDirs.Entries, (entry) => entry.Path.Replace('\\', '/').Contains("include/app", StringComparison.OrdinalIgnoreCase));
        Assert.Contains("AdditionalIncludeDirectories", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversInputsNode()
    {
        BuildRuleNode rule = fixture.DebugTree.GetMergedNode<BuildRuleNode>(fixture.AppModule.Node, (n) => n.RuleName == "gen_step", false);
        InputsNode inputs = fixture.DebugTree.GetMergedNode<InputsNode>(rule.Node, false);
        Assert.Contains(inputs.Entries, (entry) => entry.FilePath.Replace('\\', '/').EndsWith("schema/input.idl", StringComparison.OrdinalIgnoreCase));
        Assert.Contains("AdditionalInputs", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversInstallNode()
    {
        InstallNode install = fixture.DebugTree.GetNodes<InstallNode>(fixture.PluginNode.Node).Single();
        Assert.NotNull(install);
    }

    [Fact]
    public void CoversLibDirsNode()
    {
        LibDirsNode libDirs = fixture.DebugTree.GetMergedNode<LibDirsNode>(fixture.AppModule.Node);
        Assert.Contains(libDirs.Entries, (entry) => entry.Path.Replace('\\', '/').Contains("lib/app", StringComparison.OrdinalIgnoreCase));
        Assert.Contains("AdditionalLibraryDirectories", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversLinkOptionsNode()
    {
        LinkOptionsNode linkOptions = fixture.DebugTree.GetMergedNode<LinkOptionsNode>(fixture.AppModule.Node);
        Assert.False(linkOptions.IncrementalLink);
        Assert.Contains("/INCREMENTAL:NO", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversModuleNode()
    {
        IEnumerable<ModuleNode> modules = fixture.DebugTree.IndexedNodes.GetAllNodes<ModuleNode>();
        Assert.Contains(modules, (m) => m.ModuleName == "test_app");
        Assert.True(File.Exists(fixture.AppVcxprojPath));
    }

    [Fact]
    public void CoversForeachModuleExtensionDefines()
    {
        DefinesNode appDefines = fixture.DebugTree.GetMergedNode<DefinesNode>(fixture.AppModule.Node);
        Assert.Contains(appDefines.Entries, (entry) => entry.DefineName == "HE_CFG_MODULE_NAME=\"test_app\"");
        Assert.Single(appDefines.Entries, (entry) => entry.DefineName.StartsWith("HE_CFG_MODULE_KIND=", StringComparison.Ordinal));
        Assert.Contains(appDefines.Entries, (entry) => entry.DefineName == "HE_CFG_MODULE_KIND=1");

        DefinesNode libDefines = fixture.DebugTree.GetMergedNode<DefinesNode>(fixture.LibModule.Node);
        Assert.Contains(libDefines.Entries, (entry) => entry.DefineName == "HE_CFG_MODULE_NAME=\"test_lib\"");
        Assert.Contains(libDefines.Entries, (entry) => entry.DefineName == "HE_CFG_MODULE_KIND=4");

        DefinesNode toolDefines = fixture.DebugTree.GetMergedNode<DefinesNode>(fixture.ToolModule.Node);
        Assert.Contains(toolDefines.Entries, (entry) => entry.DefineName == "HE_CFG_MODULE_NAME=\"test_tool\"");
        Assert.Contains(toolDefines.Entries, (entry) => entry.DefineName == "HE_CFG_MODULE_KIND=7");
    }

    [Fact]
    public void CoversOptimizeNode()
    {
        OptimizeNode optimize = fixture.DebugTree.GetMergedNode<OptimizeNode>(fixture.AppModule.Node);
        Assert.Equal(EOptimizationLevel.On, optimize.OptimizationLevel);
        Assert.Contains("Optimization", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversOptionNode()
    {
        Assert.True(fixture.ProjectService.ProjectOptionValues.ContainsKey("feature"));
        Assert.Equal("on", fixture.ProjectService.ProjectOptionValues["feature"]);
    }

    [Fact]
    public void CoversOutputsNode()
    {
        BuildRuleNode rule = fixture.DebugTree.GetMergedNode<BuildRuleNode>(fixture.AppModule.Node, (n) => n.RuleName == "gen_step", false);
        OutputsNode outputs = fixture.DebugTree.GetMergedNode<OutputsNode>(rule.Node, false);
        Assert.Contains(outputs.Entries, (entry) => entry.FilePath.Replace('\\', '/').EndsWith("generated/generated.cpp", StringComparison.OrdinalIgnoreCase));
        Assert.Contains("Outputs", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversPlatformNode()
    {
        Assert.Equal("Win64", fixture.DebugTree.ProjectContext.Platform.PlatformName);
        Assert.Contains("Platform Name=\"Win64\"", fixture.SlnxText);
    }

    [Fact]
    public void CoversPluginNode()
    {
        Assert.Equal("unit.plugin", fixture.PluginNode.PluginName);
        Assert.Equal("1.0.0", fixture.PluginNode.Version);
        string installDir = fixture.PluginNode.GetInstallDir(fixture.DebugTree.ProjectContext);
        Assert.Contains("unit_dep-", installDir, StringComparison.OrdinalIgnoreCase);
        Assert.EndsWith(Path.Combine("dep-root"), installDir, StringComparison.OrdinalIgnoreCase);
    }
    [Fact]
    public void CoversProjectNode()
    {
        Assert.Equal("Unit Test Project", fixture.ProjectNode.ProjectName);
        Assert.True(File.Exists(fixture.SlnxPath));
    }

    [Fact]
    public void CoversPublicNode()
    {
        Assert.True(fixture.DebugTree.GetNodes<PublicNode>(fixture.LibModule.Node).Any());
    }

    [Fact]
    public void CoversRuntimeNode()
    {
        RuntimeNode runtime = fixture.DebugTree.GetMergedNode<RuntimeNode>(fixture.AppModule.Node);
        Assert.Equal(ERuntime.Release, runtime.Runtime);
        Assert.Contains("RuntimeLibrary", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversSanitizeNode()
    {
        SanitizeNode sanitize = fixture.DebugTree.GetMergedNode<SanitizeNode>(fixture.AppModule.Node);
        Assert.True(sanitize.EnableAddress);
        Assert.True(sanitize.EnableFuzzer);
        Assert.Contains("EnableASAN", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversSymbolsNode()
    {
        SymbolsNode symbols = fixture.DebugTree.GetMergedNode<SymbolsNode>(fixture.AppModule.Node);
        Assert.Equal(ESymbolsMode.On, symbols.SymbolsMode);
        Assert.Contains("GenerateDebugInformation", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversSystemNode()
    {
        SystemNode windowsSystem = fixture.DebugTree.GetMergedNode<SystemNode>(fixture.AppModule.Node, (n) => n.System == EPlatformSystem.Windows);
        Assert.Equal("10.0.22621.0", windowsSystem.Version);
        Assert.Contains("WindowsTargetPlatformVersion", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversTagsNode()
    {
        TagsNode tags = fixture.DebugTree.GetMergedNode<TagsNode>(fixture.AppModule.Node);
        Assert.Contains(tags.Entries, (entry) => entry.TagName == "app_tag");
    }

    [Fact]
    public void CoversToolsetNode()
    {
        ToolsetNode toolset = fixture.DebugTree.GetMergedNode<ToolsetNode>(fixture.AppModule.Node, (n) => n.Toolset == EToolset.MSVC);
        Assert.Equal(EToolsetArch.X86_64, toolset.Arch);
        Assert.Contains("PlatformToolset", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversWarningsNode()
    {
        WarningsNode warnings = fixture.DebugTree.GetMergedNode<WarningsNode>(fixture.AppModule.Node);
        Assert.True(warnings.AreAllWarningsFatal);
        Assert.Contains("WarningLevel", fixture.AppVcxprojText);
    }

    [Fact]
    public void CoversWhenNode()
    {
        DefinesNode defines = fixture.DebugTree.GetMergedNode<DefinesNode>(fixture.AppModule.Node);
        Assert.Contains(defines.Entries, (entry) => entry.DefineName == "FROM_WHEN_DEBUG");
        Assert.Contains(defines.Entries, (entry) => entry.DefineName == "FEATURE_ENABLED");
        Assert.DoesNotContain(defines.Entries, (entry) => entry.DefineName == "FROM_WHEN_RELEASE");
    }

}
