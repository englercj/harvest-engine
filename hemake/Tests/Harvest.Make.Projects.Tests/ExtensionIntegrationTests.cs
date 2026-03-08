// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects.Tests;

public sealed class ExtensionIntegrationTests(ExtensionIntegrationFixture fixture) : IClassFixture<ExtensionIntegrationFixture>
{
    [Fact]
    public void LoadsExtensionsFromProjectFiles()
    {
        Assert.Equal("schema_compile", fixture.ProjectService.GetNodeTraits(new Harvest.Kdl.KdlNode("schema_compile")).Name);
        Assert.Equal("bin2c_compile", fixture.ProjectService.GetNodeTraits(new Harvest.Kdl.KdlNode("bin2c_compile")).Name);
        Assert.Equal("shader_compile", fixture.ProjectService.GetNodeTraits(new Harvest.Kdl.KdlNode("shader_compile")).Name);
    }

    [Fact]
    public void SchemaCompileGeneratesExpectedResolvedModules()
    {
        DependenciesNode schemaOwnerPublicDeps = fixture.DebugTree.GetMergedNode<DependenciesNode>(fixture.SchemaOwnerModule.Node, true);
        Assert.Contains(schemaOwnerPublicDeps.Entries, (entry) => entry.DependencyName == "schema_owner__schemac_1");

        DependenciesNode generatedDeps = fixture.DebugTree.GetMergedNode<DependenciesNode>(fixture.SchemaGeneratedModule.Node);
        Assert.Contains(generatedDeps.Entries, (entry) => entry.DependencyName == "schema_dep__schemac_1");
        Assert.Contains(generatedDeps.Entries, (entry) => entry.DependencyName == "he_schemac" && entry.Kind == EDependencyKind.Order);

        BuildRuleNode buildRule = fixture.DebugTree.GetMergedNode<BuildRuleNode>(fixture.SchemaGeneratedModule.Node, (node) => node.RuleName.Contains("owner_schema", StringComparison.Ordinal), false);
        OutputsNode outputs = fixture.DebugTree.GetMergedNode<OutputsNode>(buildRule.Node);
        Assert.Contains(outputs.Entries, (entry) => entry.FilePath.EndsWith("owner_schema.hsc.h", StringComparison.Ordinal));
        Assert.Contains(outputs.Entries, (entry) => entry.FilePath.EndsWith("owner_schema.hsc.cpp", StringComparison.Ordinal));
    }

    [Fact]
    public void Bin2CCompileGeneratesExpectedResolvedModules()
    {
        DependenciesNode ownerDeps = fixture.DebugTree.GetMergedNode<DependenciesNode>(fixture.BinOwnerModule.Node);
        Assert.Contains(ownerDeps.Entries, (entry) => entry.DependencyName == "bin_owner__bin2c_1");

        BuildRuleNode buildRule = fixture.DebugTree.GetMergedNode<BuildRuleNode>(fixture.BinGeneratedModule.Node, (node) => node.RuleName.Contains("embedded", StringComparison.Ordinal), false);
        CommandNode command = fixture.DebugTree.GetMergedNode<CommandNode>(buildRule.Node, (node) => !node.GetCommandString().StartsWith("echo ", StringComparison.Ordinal), false);
        Assert.Contains("-c", command.GetCommandString());
        Assert.Contains("-n c_embedded_txt", command.GetCommandString());

        OutputsNode outputs = fixture.DebugTree.GetMergedNode<OutputsNode>(buildRule.Node);
        Assert.Contains(outputs.Entries, (entry) => entry.FilePath.EndsWith("embedded.txt.h", StringComparison.Ordinal));
    }

    [Fact]
    public void ShaderCompileGeneratesExpectedResolvedModules()
    {
        DependenciesNode ownerDeps = fixture.DebugTree.GetMergedNode<DependenciesNode>(fixture.ShaderOwnerModule.Node);
        Assert.Contains(ownerDeps.Entries, (entry) => entry.DependencyName == "shader_owner__shaderc_1");

        BuildRuleNode buildRule = fixture.DebugTree.GetMergedNode<BuildRuleNode>(fixture.ShaderGeneratedModule.Node, (node) => node.RuleName.Contains("effect", StringComparison.Ordinal), false);
        CommandNode command = fixture.DebugTree.GetMergedNode<CommandNode>(buildRule.Node, (node) => !node.GetCommandString().StartsWith("echo ", StringComparison.Ordinal), false);
        Assert.Contains("-t sm_6_0", command.GetCommandString());
        Assert.Contains("-D TEST_DEFINE", command.GetCommandString());
        Assert.Contains("-O 2", command.GetCommandString());

        OutputsNode outputs = fixture.DebugTree.GetMergedNode<OutputsNode>(buildRule.Node);
        Assert.Contains(outputs.Entries, (entry) => entry.FilePath.EndsWith("effect.shaders.h", StringComparison.Ordinal));
    }


    [Fact]
    public void SolutionGenerationAddsBuildDependenciesForGeneratedModules()
    {
        Assert.Contains("""<Project Path="projects/schema_owner.vcxproj">""", fixture.SlnxText);
        Assert.Contains("""<BuildDependency Project="projects/schema_owner__schemac_1.vcxproj" />""", fixture.SlnxText);
        Assert.Contains("""<Project Path="projects/bin_owner.vcxproj">""", fixture.SlnxText);
        Assert.Contains("""<BuildDependency Project="projects/bin_owner__bin2c_1.vcxproj" />""", fixture.SlnxText);
        Assert.Contains("""<Project Path="projects/shader_owner.vcxproj">""", fixture.SlnxText);
        Assert.Contains("""<BuildDependency Project="projects/shader_owner__shaderc_1.vcxproj" />""", fixture.SlnxText);
        Assert.Contains("""<Project Path="projects/schema_owner__schemac_1.vcxproj">""", fixture.SlnxText);
        Assert.Contains("""<BuildDependency Project="projects/schema_dep__schemac_1.vcxproj" />""", fixture.SlnxText);
    }

    [Fact]
    public void ProjectGenerationIncludesGeneratedModules()
    {
        Assert.Contains("CustomBuild", fixture.SchemaGeneratedVcxprojText);
        Assert.Contains("CustomBuild", fixture.BinGeneratedVcxprojText);
        Assert.Contains("CustomBuild", fixture.ShaderGeneratedVcxprojText);
    }
}
