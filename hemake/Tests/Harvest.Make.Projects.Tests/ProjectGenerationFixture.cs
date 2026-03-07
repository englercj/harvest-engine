// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.ProjectGenerators.vs2026;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using System.CommandLine;

namespace Harvest.Make.Projects.Tests;

public sealed class ProjectGenerationFixture : IDisposable
{
    private const string SlnxExtension = ".slnx";
    private const string VcxprojExtension = ".vcxproj";

    private readonly ILoggerFactory _loggerFactory;

    public string RootDir { get; }
    public string ProjectFilePath { get; }
    public string PluginFilePath { get; }

    public IProjectService ProjectService { get; }
    public ProjectNode ProjectNode { get; }
    public ResolvedProjectTree DebugTree { get; }

    public ModuleNode AppModule { get; }
    public ModuleNode LibModule { get; }
    public ModuleNode ToolModule { get; }
    public PluginNode PluginNode { get; }

    public string SlnxPath { get; }
    public string AppVcxprojPath { get; }
    public string LibVcxprojPath { get; }
    public string ToolVcxprojPath { get; }

    public string SlnxText { get; }
    public string AppVcxprojText { get; }
    public string LibVcxprojText { get; }
    public string ToolVcxprojText { get; }

    public ProjectGenerationFixture()
    {
        RootDir = Directory.CreateTempSubdirectory("hemake_projects_tests-").FullName;
        ProjectFilePath = WriteFile("he_project.kdl", GetProjectKdl());
        PluginFilePath = WriteFile("unit_plugin.kdl", GetPluginKdl());

        WriteSourceFiles();

        _loggerFactory = NullLoggerFactory.Instance;

        ProjectService = new ProjectService();
        new ProjectsPlugin().Startup(new ProjectServiceProvider(ProjectService));

        ProjectService.LoadProject(ProjectFilePath);

        RootCommand command = new();
        foreach (ProjectOption option in ProjectService.ProjectOptions)
        {
            command.Options.Add(option.Option);
        }

        ParseResult parseResult = command.Parse(["--feature", "on"]);
        ProjectService.ParseProject(parseResult);

        if (!ProjectService.ResolvedProjectTrees.TryGetValue(new ProjectBuildId("Debug", "Win64"), out ResolvedProjectTree? debugTree))
        {
            throw new InvalidOperationException("Failed to resolve the Debug/Win64 project tree.");
        }

        DebugTree = debugTree;
        ProjectNode = ProjectService.GetGlobalNode<ProjectNode>();

        AppModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("test_app");
        LibModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("test_lib");
        ToolModule = DebugTree.IndexedNodes.GetNode<ModuleNode>("test_tool");
        PluginNode = DebugTree.GetNodes<PluginNode>(DebugTree.ProjectNode.Node).Single();

        VS2026ProjectGeneratorService generator = new(
            ProjectService,
            _loggerFactory);
        generator.GenerateProjectFilesAsync().GetAwaiter().GetResult();

        SlnxPath = Path.Join(ProjectNode.BuildDir, $"{ProjectNode.ProjectName}{SlnxExtension}");
        AppVcxprojPath = Path.Join(ProjectNode.ProjectsDir, $"test_app{VcxprojExtension}");
        LibVcxprojPath = Path.Join(ProjectNode.ProjectsDir, $"test_lib{VcxprojExtension}");
        ToolVcxprojPath = Path.Join(ProjectNode.ProjectsDir, $"test_tool{VcxprojExtension}");

        SlnxText = File.ReadAllText(SlnxPath);
        AppVcxprojText = File.ReadAllText(AppVcxprojPath);
        LibVcxprojText = File.ReadAllText(LibVcxprojPath);
        ToolVcxprojText = File.ReadAllText(ToolVcxprojPath);
    }

    public void Dispose()
    {
        _loggerFactory.Dispose();

        try
        {
            if (Directory.Exists(RootDir))
            {
                Directory.Delete(RootDir, true);
            }
        }
        catch (IOException)
        {
            // Ignore cleanup errors in tests.
        }
        catch (UnauthorizedAccessException)
        {
            // Ignore cleanup errors in tests.
        }
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
        WriteFile("src/main.cpp", "int main() { return 0; }");
        WriteFile("src/lib.cpp", "int lib() { return 1; }");
        WriteFile("src/pch.cpp", "#include \"pch.h\"");
        WriteFile("include/lib.h", "#pragma once");
        WriteFile("include/project/pch.h", "#pragma once");
        WriteFile("include/app/pch.h", "#pragma once");
        WriteFile("include/libpublic/libpublic.h", "#pragma once");
        WriteFile("res/app.rc", "1 VERSIONINFO");
        WriteFile("schema/input.idl", "interface ITest {};");
        WriteFile("natvis/debug.natvis", "<AutoVisualizer />");
        WriteFile("app/app.manifest", "<assembly />");
        WriteFile("assets/copy.dat", "copy");
        WriteFile("assets/tool.txt", "tool");
    }

    private static string GetProjectKdl()
    {
        return """
            project "Unit Test Project" start=test_app build_dir="${project.file_path:dirname}/.build" installs_dir="${project.file_path:dirname}/.build/installs" projects_dir="${project.file_path:dirname}/.build/projects" {
                option feature type=string default="on" help="Enable feature tests"

                configuration Debug
                configuration Release

                platform Win64 arch=x86_64 system=windows toolset=msvc

                build_output bin_dir=".build/bin" gen_dir=".build/gen" lib_dir=".build/lib" obj_dir=".build/obj"
                build_options clr=off mfc=dynamic atl=static dpiawareness=high_permonitor pch_include="pch.h" pch_source="./src/pch.cpp" rtti=#true run_code_analysis=#true run_clang_tidy=#true omit_default_lib=#false omit_frame_pointers=#false openmp=#true {
                    "/Zc:preprocessor"
                }
                codegen avx2
                defines {
                    PROJECT_DEF
                }
                dialect c=c17 cpp=cpp20 csharp=cs12
                exceptions off
                external warnings=extra fatal=#true angle_brackets=#true
                floating_point precise exceptions=#true
                include_dirs external=#true {
                    "./include/project"
                }
                lib_dirs system=#true {
                    "./lib/system"
                }
                link_options incremental=#false {
                    "/WX"
                }
                optimize speed lto=on inlining=on just_my_code=#false
                runtime debug static=#false
                sanitize address=#true
                symbols on
                system windows version="10.0.22621.0"
                system dotnet version="net9.0"
                tags {
                    global_tag
                }
                toolset msvc arch=x86_64 edit_and_continue=#false fast_up_to_date_check=#false multiprocess=#true path="C:/Toolchain" version="14.40.33807"
                warnings level=extra fatal=#true {
                    "4996"
                    "4700"
                }

                when configuration=Debug {
                    defines {
                        FROM_WHEN_DEBUG
                    }
                }
                when configuration=Release {
                    defines {
                        FROM_WHEN_RELEASE
                    }
                }

                import "./unit_plugin.kdl"
            }
            """;
    }

    private static string GetPluginKdl()
    {
        return """
            plugin unit.plugin version="1.0.0" license="MIT" {
                authors {
                    "Jane Doe" email="jane@example.com"
                }

                install {
                    fetch archive url="https://example.com/unit_dep.zip" archive_format=zip base_dir="dep-root" install_dir_priority=5
                }

                module test_lib kind=lib_static group="libs/core" target_name="test_lib_target" make_map_file=#true {
                    public {
                        defines {
                            LIB_PUBLIC_DEF
                        }
                        include_dirs {
                            "./include/libpublic"
                        }
                        lib_dirs {
                            "./lib/public"
                        }
                    }

                    files {
                        "./src/lib.cpp"
                        "./include/lib.h" action=include
                    }

                    dependencies {
                        "kernel32" kind=system
                    }
                }

                module test_tool kind=custom group="tools" {
                    files {
                        "./assets/tool.txt" action=copy
                    }

                    dependencies {
                        "./assets/tool.txt" kind=file
                    }
                }

                module test_app kind=app_console group="apps" entrypoint="mainCRTStartup" {
                    build_rule "gen_step" message="Run codegen" link_output=#true {
                        command "cmd.make_dir" "./generated"
                        inputs {
                            "./schema/input.idl"
                        }
                        outputs {
                            "./generated/generated.cpp"
                        }
                    }

                    build_event prebuild message="Prebuild Step" {
                        command "cmd.make_dir" "./prebuild"
                        inputs {
                            "./schema/input.idl"
                        }
                        outputs {
                            "./prebuild/stamp.txt"
                        }
                    }

                    files {
                        "./src/main.cpp"
                        "./src/pch.cpp" action=build build_rule=cpp
                        "./res/app.rc" action=resource
                        "./schema/input.idl" action=build build_rule="gen_step"
                        "./natvis/debug.natvis" action=natvis
                        "./app/app.manifest" action=manifest
                        "./assets/copy.dat" action=copy
                    }

                    build_options pch_include="pch.h" pch_source="./src/pch.cpp" clr=off mfc=dynamic atl=static dpiawareness=high run_code_analysis=#true run_clang_tidy=#true openmp=#true omit_default_lib=#true omit_frame_pointers=#true rtti=#true {
                        "/fp:fast"
                    }
                    codegen avx2
                    defines {
                        APP_DEF
                    }
                    dependencies {
                        test_lib
                        test_tool kind=order
                        "user32" kind=system
                    }
                    dialect c=c11 cpp=cpp20
                    exceptions seh
                    floating_point strict exceptions=#true
                    include_dirs {
                        "./include/app"
                    }
                    lib_dirs {
                        "./lib/app"
                    }
                    link_options incremental=#false {
                        "/INCREMENTAL:NO"
                    }
                    optimize on lto=on inlining=explicit function_level_linking=#true string_pooling=#true intrinsics=#true just_my_code=#false
                    runtime release static=#true
                    sanitize address=#true fuzzer=#true
                    symbols on embed=#false
                    tags {
                        app_tag
                    }
                    warnings level=all fatal=#true {
                        "4996"
                    }

                    when option=feature {
                        defines {
                            FEATURE_ENABLED
                        }
                    }
                }
            }
            """;
    }

    private sealed class ProjectServiceProvider(IProjectService projectService) : IServiceProvider
    {
        public object? GetService(Type serviceType)
        {
            return serviceType == typeof(IProjectService) ? projectService : null;
        }
    }
}
