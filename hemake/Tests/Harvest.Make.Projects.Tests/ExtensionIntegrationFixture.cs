// Copyright Chad Engler

using Harvest.Common.Services;
using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.ProjectGenerators.vs2026;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using System.CommandLine;

namespace Harvest.Make.Projects.Tests;

public sealed class ExtensionIntegrationFixture : IDisposable
{
    private readonly ILoggerFactory _loggerFactory = NullLoggerFactory.Instance;
    private readonly ExtensionService _extensionService;
    private readonly IServiceProvider _services;

    public string RootDir { get; }
    public string ProjectFilePath { get; }
    public IProjectService ProjectService { get; }
    public ProjectNode ProjectNode { get; }
    public ResolvedProjectTree DebugTree { get; }

    public ModuleNode SchemaOwnerModule { get; }
    public ModuleNode SchemaGeneratedModule { get; }
    public ModuleNode BinOwnerModule { get; }
    public ModuleNode BinGeneratedModule { get; }
    public ModuleNode ShaderOwnerModule { get; }
    public ModuleNode ShaderGeneratedModule { get; }

    public string SlnxText { get; }
    public string SchemaOwnerVcxprojText { get; }
    public string BinOwnerVcxprojText { get; }
    public string ShaderOwnerVcxprojText { get; }
    public string SchemaGeneratedVcxprojText { get; }
    public string BinGeneratedVcxprojText { get; }
    public string ShaderGeneratedVcxprojText { get; }

    public ExtensionIntegrationFixture()
    {
        RootDir = Directory.CreateTempSubdirectory("hemake_extension_tests-").FullName;
        ProjectFilePath = WriteFile("he_project.kdl", GetProjectKdl());
        WriteFile("plugin.kdl", GetPluginKdl());
        WriteSourceFiles();

        ProjectService = new ProjectService();
        new ProjectsExtension().Startup(new ProjectServiceProvider(ProjectService));
        ProjectService.LoadProject(ProjectFilePath);

        _extensionService = new ExtensionService(_loggerFactory.CreateLogger<ExtensionService>());
        LoadExtensions();

        _services = new ProjectServiceProvider(ProjectService);
        _extensionService.Startup(_services);

        RootCommand command = new();
        foreach (ProjectOption option in ProjectService.ProjectOptions)
        {
            command.Options.Add(option.Option);
        }

        ParseResult parseResult = command.Parse([]);
        ProjectService.ParseProject(parseResult);

        if (!ProjectService.ResolvedProjectTrees.TryGetValue(new ProjectBuildId("Debug", "Win64"), out ResolvedProjectTree? debugTree))
        {
            throw new InvalidOperationException("Failed to resolve the Debug/Win64 project tree.");
        }

        DebugTree = debugTree;
        ProjectNode = ProjectService.GetGlobalNode<ProjectNode>();

        SchemaOwnerModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("schema_owner");
        SchemaGeneratedModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("schema_owner__schemac_1");
        BinOwnerModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("bin_owner");
        BinGeneratedModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("bin_owner__bin2c_1");
        ShaderOwnerModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("shader_owner");
        ShaderGeneratedModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("shader_owner__shaderc_1");

        VS2026ProjectGeneratorService generator = new(ProjectService, _loggerFactory);
        generator.GenerateProjectFilesAsync().GetAwaiter().GetResult();

        SlnxText = File.ReadAllText(Path.Join(ProjectNode.BuildDir, $"{ProjectNode.ProjectName}.slnx"));
        SchemaOwnerVcxprojText = ReadProject("schema_owner");
        BinOwnerVcxprojText = ReadProject("bin_owner");
        ShaderOwnerVcxprojText = ReadProject("shader_owner");
        SchemaGeneratedVcxprojText = ReadProject("schema_owner__schemac_1");
        BinGeneratedVcxprojText = ReadProject("bin_owner__bin2c_1");
        ShaderGeneratedVcxprojText = ReadProject("shader_owner__shaderc_1");
    }

    public void Dispose()
    {
        _extensionService.Shutdown();
        if (_services is IDisposable disposable)
        {
            disposable.Dispose();
        }

        try
        {
            if (Directory.Exists(RootDir))
            {
                Directory.Delete(RootDir, true);
            }
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
        }
    }

    private void LoadExtensions()
    {
        DotnetBuildService dotnetBuildService = new(_loggerFactory.CreateLogger<DotnetBuildService>());
        HashSet<string> seenProjectFiles = [];
        List<Task<DotnetBuildResult>> pendingBuilds = [];

        foreach (KdlNode node in ProjectService.ProjectDocument.GetNodesByName(ModuleNode.NodeTraits.Name))
        {
            ModuleNode module = new(node);
            if (module.Kind != EModuleKind.HarvestMakeExtension)
            {
                continue;
            }

            if (!node.TryGetValue("project_file", out string? projectFilePath) || string.IsNullOrWhiteSpace(projectFilePath))
            {
                throw new InvalidOperationException($"Module '{node.Name}' is missing 'project_file'.");
            }

            string fullProjectFilePath = Path.IsPathRooted(projectFilePath)
                ? Path.GetFullPath(projectFilePath)
                : Path.GetFullPath(projectFilePath, Path.GetDirectoryName(node.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory());
            if (!seenProjectFiles.Add(fullProjectFilePath))
            {
                continue;
            }

            pendingBuilds.Add(dotnetBuildService.BuildProjectAsync(fullProjectFilePath));
        }

        foreach (DotnetBuildResult buildResult in Task.WhenAll(pendingBuilds).GetAwaiter().GetResult())
        {
            _extensionService.LoadExtensionsFromAssemblyFile(buildResult.AssemblyPath);
        }
    }

    private string ReadProject(string moduleName)
    {
        return File.ReadAllText(Path.Join(ProjectNode.ProjectsDir, moduleName + ".vcxproj"));
    }

    private string WriteFile(string relativePath, string content)
    {
        string fullPath = Path.Join(RootDir, relativePath);
        string? dir = Path.GetDirectoryName(fullPath);
        if (!string.IsNullOrEmpty(dir))
        {
            Directory.CreateDirectory(dir);
        }

        File.WriteAllText(fullPath, content);
        return fullPath;
    }

    private void WriteSourceFiles()
    {
        WriteFile("schema/include/owner_schema.hsc", "struct OwnerSchema {}\n");
        WriteFile("schema/src/owner.cpp", "int owner() { return 0; }\n");
        WriteFile("schema_dep/include/dep_schema.hsc", "struct DepSchema {}\n");
        WriteFile("schema_dep/src/dep.cpp", "int dep() { return 0; }\n");
        WriteFile("schema_runtime/include/runtime.h", "#pragma once\n");
        WriteFile("schema_runtime/src/runtime.cpp", "int runtime_schema() { return 0; }\n");
        WriteFile("tools/schemac/main.cpp", "int main() { return 0; }\n");
        WriteFile("bin/assets/embedded.txt", "embedded\n");
        WriteFile("bin/src/bin_owner.cpp", "int bin_owner() { return 0; }\n");
        WriteFile("tools/bin2c/main.cpp", "int main() { return 0; }\n");
        WriteFile("shader/src/effect.slang", "[shader(\"compute\")] void main() {}\n");
        WriteFile("shader/src/shader_owner.cpp", "int shader_owner() { return 0; }\n");
        WriteFile("tools/shaderc/main.cpp", "int main() { return 0; }\n");
        WriteFile("core/src/core.cpp", "int core() { return 0; }\n");
    }

    private static string GetProjectKdl()
    {
        return """
            project "HE Make Extension Tests" build_dir="${project.file_path:dirname}/.build" installs_dir="${project.file_path:dirname}/.build/installs" projects_dir="${project.file_path:dirname}/.build/projects" {
                configuration Debug
                platform Win64 arch=x86_64 system=windows toolset=msvc
                build_output bin_dir=".build/bin" gen_dir=".build/gen" lib_dir=".build/lib" obj_dir=".build/obj"
                codegen avx2
                import "./plugin.kdl"
            }
            """;
    }

    private static string GetPluginKdl()
    {
        string repoRoot = FindRepoRoot();
        string schemaProject = Path.Combine(repoRoot, "plugins", "schema", "Harvest.Make.Schema.csproj").Replace('\\', '/');
        string bin2cProject = Path.Combine(repoRoot, "plugins", "bin2c", "Harvest.Make.Bin2C.csproj").Replace('\\', '/');
        string shaderProject = Path.Combine(repoRoot, "plugins", "rhi", "Harvest.Make.Shader.csproj").Replace('\\', '/');

        return $$"""
            plugin test.extensions version="1.0.0" license="MIT" {
                authors { "Unit Test" email="unit@test" }

                module test_schema_hemake kind=hemake_extension group="_internal/hemake" project_file="{{schemaProject}}" {}
                module test_bin2c_hemake kind=hemake_extension group="_internal/hemake" project_file="{{bin2cProject}}" {}
                module test_shader_hemake kind=hemake_extension group="_internal/hemake" project_file="{{shaderProject}}" {}

                module he_core kind=lib_static {
                    codegen avx2
                    files { "core/src/core.cpp" }
                }

                module he_schema kind=lib_static {
                    codegen avx2
                    files { "schema_runtime/src/runtime.cpp" }
                    public {
                        include_dirs { "schema_runtime/include" }
                    }
                }

                module he_schemac kind=app_console {
                    codegen avx2
                    files { "tools/schemac/main.cpp" }
                    dependencies { he_core; he_schema }
                }

                module he_bin2c kind=app_console {
                    codegen avx2
                    files { "tools/bin2c/main.cpp" }
                    dependencies { he_core }
                }

                module he_shaderc kind=app_console {
                    codegen avx2
                    files { "tools/shaderc/main.cpp" }
                    dependencies { he_core }
                }

                module schema_dep kind=lib_static {
                    codegen avx2
                    files { "schema_dep/src/dep.cpp" }
                    public {
                        include_dirs { "schema_dep/include" }
                    }

                    schema_compile "c++" scope=public {
                        files { "schema_dep/include/dep_schema.hsc" }
                        include_dirs { "schema_dep/include" }
                    }
                }

                module schema_owner kind=lib_static {
                    codegen avx2
                    files { "schema/src/owner.cpp" }
                    public {
                        include_dirs { "schema/include" }
                    }

                    schema_compile "c++" scope=public {
                        files { "schema/include/owner_schema.hsc" }
                        include_dirs { "schema/include" }
                        dependencies { schema_dep }
                    }
                }

                module bin_owner kind=lib_static {
                    codegen avx2
                    files { "bin/src/bin_owner.cpp" }
                    bin2c_compile scope=private compress=#true {
                        files { "bin/assets/embedded.txt" }
                        include_dirs { "bin/assets" }
                    }
                }

                module shader_owner kind=lib_static {
                    codegen avx2
                    files { "shader/src/shader_owner.cpp" }
                    shader_compile "sm_6_0" scope=private optimize="2" {
                        files { "shader/src/effect.slang" }
                        include_dirs { "shader/src" }
                        defines { TEST_DEFINE }
                    }
                }
            }
            """;
    }

    private static string FindRepoRoot()
    {
        string? current = AppContext.BaseDirectory;
        while (!string.IsNullOrEmpty(current))
        {
            if (File.Exists(Path.Combine(current, "he_project.kdl")))
            {
                return current;
            }

            current = Directory.GetParent(current)?.FullName;
        }

        throw new InvalidOperationException("Failed to find the repository root.");
    }

    private sealed class ProjectServiceProvider(IProjectService projectService) : IServiceProvider
    {
        public object? GetService(Type serviceType)
        {
            return serviceType == typeof(IProjectService) ? projectService : null;
        }
    }
}
