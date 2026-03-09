// Copyright Chad Engler

using Harvest.Common.Extensions;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.Logging;
using System.Text;
using System.Xml;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

internal class VcxprojGenerator(string platformToolset, IProjectService projectService, ILogger<VcxprojGenerator> logger)
{
    public const string ProjectExtension = ".vcxproj";
    public const string FiltersExtension = ".vcxproj.filters";
    public const string XmlNamespace = "http://schemas.microsoft.com/developer/msbuild/2003";

    private readonly IProjectService _projectService = projectService;
    private readonly ILogger _logger = logger;
    private string _moduleName = "";
    private string _outputPath = "";
    private List<IVisualStudioFileGroup> _fileGroups = [];

    public async Task GenerateAsync(string moduleName)
    {
        _moduleName = moduleName;

        ProjectNode project = _projectService.GetGlobalNode<ProjectNode>();
        Directory.CreateDirectory(project.ProjectsDir);

        _outputPath = Path.Join(project.ProjectsDir, $"{moduleName}{ProjectExtension}");
        _logger.LogDebug("Generating project file: {ProjectPath}", _outputPath);

        CreateFileGroups();

        await GenerateProjectFileAsync();
        await GenerateFiltersFileAsync(project.ProjectsDir, moduleName);
        await GenerateUserFileAsync(project.ProjectsDir, moduleName);
    }

    private async Task GenerateProjectFileAsync()
    {
        await using FileStream stream = new(_outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
        await using XmlWriter writer = XmlWriter.Create(stream, new XmlWriterSettings
        {
            Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false),
            Async = true,
            Indent = true,
            IndentChars = "  ",
            NewLineChars = Environment.NewLine,
            OmitXmlDeclaration = false,
        });

        writer.WriteStartDocument();
        writer.WriteStartElement("Project", XmlNamespace);
        writer.WriteAttributeString("DefaultTargets", "Build");

        WriteConfigurations(writer);
        WriteGlobals(writer);
        WriteImportDefaultProperties(writer);
        WriteConfigurationProperties(writer);
        WriteImportProperties(writer);
        WriteUserMacros(writer);
        WriteOutputProperties(writer);
        WriteItemDefinitionGroups(writer);
        // TODO: managed assembly references (<ItemGroup><Reference Include="..." /></ItemGroup>)
        WriteFiles(writer);
        WriteProjectReferences(writer);
        WriteLanguageTargetImports(writer);
        WriteExtensionTargetImports(writer);
        // TODO: nuget references for Visual Studio to restore, is this needed?
        // Right now hemake downloads the nuget packages to .build/ & sets up refs.

        writer.WriteEndElement();
        writer.WriteEndDocument();
    }

    private async Task GenerateFiltersFileAsync(string projectsDir, string moduleName)
    {
        string outputPath = Path.Join(projectsDir, $"{moduleName}{FiltersExtension}");

        await using FileStream stream = new(outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
        await using XmlWriter writer = XmlWriter.Create(stream, new XmlWriterSettings
        {
            Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false),
            Async = true,
            Indent = true,
            IndentChars = "  ",
            NewLineChars = Environment.NewLine,
            OmitXmlDeclaration = false,
        });

        HashSet<string> seenVirtualDirs = [];

        writer.WriteStartDocument();
        writer.WriteStartElement("Project", XmlNamespace);
        writer.WriteAttributeString("ToolsVersion", "4.0"); // vs2022 filter version

        writer.WriteStartElement("ItemGroup");

        foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
        {
            foreach (IVisualStudioFileGroup.FileEntry file in fileGroup.Files)
            {
                if (seenVirtualDirs.Add(file.VirtualPath))
                {
                    writer.WriteStartElement("Filter");
                    writer.WriteAttributeString("Include", file.VirtualPath);

                    Guid uniqueId = VisualStudioUtils.CreateGuidForFilter(file.VirtualPath);
                    writer.WriteElementString("UniqueIdentifier", $"{{{uniqueId}}}");

                    writer.WriteEndElement();
                }
            }
        }

        writer.WriteEndElement();

        foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
        {
            fileGroup.WriteFilters(writer);
        }

        writer.WriteEndElement();
        writer.WriteEndDocument();
    }

    private Task GenerateUserFileAsync(string projectsDir, string moduleName)
    {
        // TODO: debugger settings: command, args, flavor, type, working dir, environment variables, etc.

        //_outputPath = Path.Join(projectsDir, $"{moduleName}{ProjectExtension}.user");

        //await using FileStream stream = new(_outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
        //await using XmlWriter writer = XmlWriter.Create(stream, new XmlWriterSettings { Encoding = Encoding.UTF8, Indent = true });

        //writer.WriteStartDocument();
        //writer.WriteStartElement("Project", XmlNamespace);
        //writer.WriteAttributeString("ToolsVersion", "Current"); // vs2022 user version

        //writer.WriteStartElement("PropertyGroup");
        //writer.WriteEndElement();

        //writer.WriteEndElement();
        //writer.WriteEndDocument();

        return Task.CompletedTask;
    }

    private string GetPath(string path)
    {
        return VisualStudioUtils.TranslatePath(_outputPath, path);
    }

    private IEnumerable<(ResolvedProjectTree ProjectTree, ModuleNode Module, string ArchName)> EnumerateModuleConfigs()
    {
        foreach ((ResolvedProjectTree projectTree, string archName) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            if (projectTree.IndexedNodes.TryGetNode(_moduleName, out ModuleNode? module))
            {
                yield return (projectTree, module, archName);
            }
        }
    }

    private bool TryGetFirstModuleConfig(out ResolvedProjectTree? projectTree, out ModuleNode? module, out string archName)
    {
        foreach ((ResolvedProjectTree tree, ModuleNode treeModule, string treeArchName) in EnumerateModuleConfigs())
        {
            projectTree = tree;
            module = treeModule;
            archName = treeArchName;
            return true;
        }

        projectTree = null;
        module = null;
        archName = "";
        return false;
    }

    private void WriteConfigurations(XmlWriter writer)
    {
        writer.WriteStartElement("ItemGroup");
        writer.WriteAttributeString("Label", "ProjectConfigurations");

        HashSet<string> configNames = [];
        HashSet<string> platformNames = [];

        foreach ((ResolvedProjectTree project, _, string archName) in EnumerateModuleConfigs())
        {
            string configName = VisualStudioUtils.GetConfigName(project);
            configNames.Add(configName);
            platformNames.Add(archName);
        }

        foreach (string configName in configNames)
        {
            foreach (string platformName in platformNames)
            {
                writer.WriteStartElement("ProjectConfiguration");
                writer.WriteAttributeString("Include", $"{configName}|{platformName}");
                writer.WriteElementString("Configuration", configName);
                writer.WriteElementString("Platform", platformName);
                writer.WriteEndElement();
            }
        }

        writer.WriteEndElement();
    }

    private void WriteGlobals(XmlWriter writer)
    {
        if (!TryGetFirstModuleConfig(out ResolvedProjectTree? firstProjectTree, out ModuleNode? module, out _))
        {
            throw new InvalidOperationException($"No module named '{_moduleName}' was found in resolved project trees.");
        }

        ResolvedProjectTree resolvedProjectTree = firstProjectTree!;
        ModuleNode resolvedModule = module!;

        bool isWindows = EnumerateModuleConfigs().Any((entry) => entry.ProjectTree.ProjectContext.Platform.System == EPlatformSystem.Windows);
        BuildOptionsNode buildOptions = resolvedProjectTree.GetMergedNode<BuildOptionsNode>(resolvedModule.Node);
        SystemNode dotnetSystemNode = resolvedProjectTree.GetMergedNode<SystemNode>(resolvedModule.Node, (n) => n.System == EPlatformSystem.DotNet);
        ToolsetNode toolsetNode = resolvedProjectTree.GetMergedNode<ToolsetNode>(resolvedModule.Node, (n) => n.Toolset == EToolset.MSVC);
        SystemNode windowsSystemNode = resolvedProjectTree.GetMergedNode<SystemNode>(resolvedModule.Node, (n) => n.System == EPlatformSystem.Windows);

        writer.WriteStartElement("PropertyGroup");
        writer.WriteAttributeString("Label", "Globals");

        writer.WriteElementString("ProjectGuid", ModuleGroupTree.GetModuleGuid(resolvedModule));
        writer.WriteElementString("ProjectName", resolvedModule.ModuleName);

        // TODO: Is this needed?
        // I think we'd need this if we let users specify an explicit virtual path for files because
        // that might result in multiple files of the same name in the same virtual dir?
        //writer.WriteElementString("IgnoreWarnCompileDuplicatedFilename", "true");

        if (isWindows)
        {
            bool isManaged = VisualStudioUtils.IsManaged(resolvedModule, buildOptions);
            bool isClrMixed = VisualStudioUtils.IsClrMixed(resolvedModule, buildOptions);

            if ((isManaged || isClrMixed) && !string.IsNullOrWhiteSpace(dotnetSystemNode.Version))
            {
                writer.WriteElementString("TargetFramework", dotnetSystemNode.Version);
            }

            writer.WriteElementString("Keyword", isManaged ? "ManagedCProj" : "Win32Proj");
            writer.WriteElementString("RootNamespace", resolvedModule.ModuleName);
        }

        switch (toolsetNode.Arch)
        {
            case EToolsetArch.X86:
                writer.WriteElementString("PreferredToolArchitecture", "x86");
                break;
            case EToolsetArch.X86_64:
                writer.WriteElementString("PreferredToolArchitecture", "x64");
                break;
            case EToolsetArch.Default:
                break;
        }

        if (!string.IsNullOrWhiteSpace(windowsSystemNode.Version) && !windowsSystemNode.IsLatestVersion)
        {
            writer.WriteElementString("WindowsTargetPlatformVersion", windowsSystemNode.Version);
        }

        writer.WriteElementBoolIfTrue("DisableFastUpToDateCheck", !toolsetNode.FastUpToDateCheck);

        writer.WriteEndElement();
    }

    private static void WriteImportDefaultProperties(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
        writer.WriteEndElement();
    }

    private void WriteConfigurationProperties(XmlWriter writer)
    {
        foreach ((ResolvedProjectTree projectTree, ModuleNode module, string archName) in EnumerateModuleConfigs())
        {
            ProjectContext context = projectTree.ProjectContext;
            ConfigurationNode configuration = context.Configuration;
            PlatformNode platform = context.Platform;

            BuildOptionsNode buildOptions = projectTree.GetMergedNode<BuildOptionsNode>(module.Node);
            ToolsetNode toolsetNode = projectTree.GetMergedNode<ToolsetNode>(module.Node, (n) => n.Toolset == platform.Toolset);
            string configCondition = VisualStudioUtils.GetConfigCondition(projectTree, archName);

            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", configCondition);
            writer.WriteAttributeString("Label", "Configuration");

            writer.WriteElementString("ConfigurationType", VisualStudioUtils.ModuleKindNames[module.Kind]);
            writer.WriteElementString("CharacterSet", "Unicode");

            if (platform.Toolset == EToolset.Clang)
            {
                writer.WriteElementString("PlatformToolset", "ClangCL");
                if (toolsetNode.Path is string toolsetPath)
                {
                    writer.WriteElementString("LLVMInstallDir", toolsetPath);
                }
                if (toolsetNode.Version is string toolsetVersion)
                {
                    writer.WriteElementString("LLVMToolsVersion", toolsetVersion);
                }
            }
            else
            {
                if (toolsetNode.Version is string toolsetVersion)
                {
                    writer.WriteElementString("VCToolsVersion", toolsetVersion);
                }
                writer.WriteElementString("PlatformToolset", platformToolset);
            }

            if (module.Kind != EModuleKind.Custom && module.Kind != EModuleKind.Content)
            {
                RuntimeNode runtimeNode = projectTree.GetMergedNode<RuntimeNode>(module.Node);
                if (runtimeNode.Runtime == ERuntime.Debug || runtimeNode.Runtime == ERuntime.Default && configuration.ConfigName == "Debug")
                {
                    writer.WriteElementString("UseDebugLibraries", "true");
                }
                else
                {
                    writer.WriteElementString("UseDebugLibraries", "false");
                }

                if (buildOptions.MfcMode == EBuildMfcMode.Static)
                {
                    writer.WriteElementString("UseOfMfc", "Static");
                }
                else if (buildOptions.MfcMode == EBuildMfcMode.Dynamic)
                {
                    writer.WriteElementString("UseOfMfc", "Dynamic");
                }

                if (buildOptions.AtlMode == EBuildAtlMode.Static)
                {
                    writer.WriteElementString("UseOfAtl", "Static");
                }
                else if (buildOptions.AtlMode == EBuildAtlMode.Dynamic)
                {
                    writer.WriteElementString("UseOfAtl", "Dynamic");
                }

                switch (buildOptions.ClrMode)
                {
                    case EBuildClrMode.On:
                        writer.WriteElementString("CLRSupport", "true");
                        break;
                    case EBuildClrMode.Off:
                        // Always enabled for C# code.
                        if (module.Language == EModuleLanguage.CSharp)
                        {
                            writer.WriteElementString("CLRSupport", "true");
                        }
                        break;
                    case EBuildClrMode.NetCore:
                        writer.WriteElementString("CLRSupport", "NetCore");
                        break;
                }

                SanitizeNode sanitize = projectTree.GetMergedNode<SanitizeNode>(module.Node);
                if (sanitize.EnableAddress)
                {
                    writer.WriteElementString("EnableASAN", "true");
                }
                if (sanitize.EnableFuzzer)
                {
                    writer.WriteElementString("EnableFuzzer", "true");
                }

                OptimizeNode optimize = projectTree.GetMergedNode<OptimizeNode>(module.Node);
                if (optimize.LinkTimeOptimizationLevel == ELinkTimeOptimizationLevel.On)
                {
                    writer.WriteElementString("WholeProgramOptimization", "true");
                }

                if (platform.System == EPlatformSystem.Windows)
                {
                    if (platform.Arch == EPlatformArch.Arm)
                    {
                        writer.WriteElementString("WindowsSDKDestopARMSupport", "true");
                    }
                    else if (platform.Arch == EPlatformArch.Arm64)
                    {
                        writer.WriteElementString("WindowsSDKDesktopARM64Support", "true");
                    }
                }
            }
            else
            {
                BuildOutputNode buildOutput = projectTree.GetMergedNode<BuildOutputNode>(module.Node);
                string outDir = GetPath(module.GetTargetDir(buildOutput));
                writer.WriteElementString("OutDir", $"{outDir}\\");

                string objDir = GetPath(module.GetObjDir(buildOutput));
                writer.WriteElementString("IntDir", $"{objDir}\\");
            }

            writer.WriteEndElement();
        }
    }

    private void WriteImportProperties(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
        writer.WriteEndElement();

        WriteExtensionSettings(writer);

        foreach ((ResolvedProjectTree projectTree, _, string archName) in EnumerateModuleConfigs())
        {
            string configCondition = VisualStudioUtils.GetConfigCondition(projectTree, archName);

            writer.WriteStartElement("ImportGroup");
            writer.WriteAttributeString("Label", "PropertySheets");
            writer.WriteAttributeString("Condition", configCondition);

            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
            writer.WriteAttributeString("Condition", $"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
            writer.WriteAttributeString("Label", "LocalAppDataPlatform");
            writer.WriteEndElement();

            writer.WriteEndElement();
        }
    }

    private static void WriteUserMacros(XmlWriter writer)
    {
        writer.WriteStartElement("PropertyGroup");
        writer.WriteAttributeString("Label", "UserMacros");
        writer.WriteEndElement();
    }

    private void WriteOutputProperties(XmlWriter writer)
    {
        foreach ((ResolvedProjectTree projectTree, ModuleNode module, string archName) in EnumerateModuleConfigs())
        {
            ProjectContext context = projectTree.ProjectContext;
            BuildOutputNode buildOutput = projectTree.GetMergedNode<BuildOutputNode>(module.Node);
            BuildOptionsNode buildOptions = projectTree.GetMergedNode<BuildOptionsNode>(module.Node);
            LinkOptionsNode linkOptions = projectTree.GetMergedNode<LinkOptionsNode>(module.Node);
            OptimizeNode optimize = projectTree.GetMergedNode<OptimizeNode>(module.Node);
            ToolsetNode toolset = projectTree.GetMergedNode<ToolsetNode>(module.Node, (n) => n.Toolset == context.Platform.Toolset);
            string configCondition = VisualStudioUtils.GetConfigCondition(projectTree, archName);

            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", configCondition);

            string outDir = GetPath(module.GetTargetDir(buildOutput));
            writer.WriteElementString("OutDir", $"{outDir}\\");

            string objDir = GetPath(module.GetObjDir(buildOutput));
            writer.WriteElementString("IntDir", $"{objDir}\\");

            if (module.Kind != EModuleKind.Custom)
            {
                if (module.Kind != EModuleKind.LibStatic)
                {
                    bool canLinkIncremental = VisualStudioUtils.CanLinkIncrememntal(module, linkOptions, optimize);
                    writer.WriteElementBool("LinkIncremental", canLinkIncremental);
                }

                if (module.Kind == EModuleKind.LibShared)
                {
                    writer.WriteElementBool("IgnoreImportLibrary", module.MakeImportLib);
                }

                writer.WriteElementString("TargetName", module.TargetName);

                string targetExt = module.GetTargetExtension(context);
                if (!string.IsNullOrEmpty(targetExt))
                {
                    writer.WriteElementString("TargetExt", targetExt);
                }
                else
                {
                    writer.WriteStartElement("TargetExt");
                    writer.WriteEndElement();
                }

                IncludeDirsNode includeDirs = projectTree.GetMergedNode<IncludeDirsNode>(module.Node, true);
                IEnumerable<string> externalIncludePaths = includeDirs.Entries.Where((entry) => entry.IsExternal).Select((entry) => GetPath(entry.Path));
                VisualStudioUtils.WriteArrayElement(writer, externalIncludePaths, "ExternalIncludePath", "$(ExternalIncludePath)");

                LibDirsNode libDirs = projectTree.GetMergedNode<LibDirsNode>(module.Node, true);
                IEnumerable<string> systemLibPaths = libDirs.Entries.Where((entry) => entry.IsSystem).Select((entry) => GetPath(entry.Path));
                VisualStudioUtils.WriteArrayElement(writer, systemLibPaths, "LibraryPath", "%(LibraryPath)");

                if (!module.MakeExeManifest)
                {
                    writer.WriteElementString("GenerateManifest", "false");
                }

                writer.WriteElementBoolIfTrue("EnableClangTidyCodeAnalysis", buildOptions.RunClangTidy);
                writer.WriteElementBoolIfTrue("RunCodeAnalysis", buildOptions.RunCodeAnalysis);
            }

            writer.WriteEndElement();
        }
    }

    private void WriteItemDefinitionGroups(XmlWriter writer)
    {
        foreach ((ResolvedProjectTree projectTree, ModuleNode module, string archName) in EnumerateModuleConfigs())
        {
            ProjectContext context = projectTree.ProjectContext;
            PlatformNode platform = context.Platform;

            ToolsetNode toolset = projectTree.GetMergedNode<ToolsetNode>(module.Node, (n) => n.Toolset == platform.Toolset);
            string configCondition = VisualStudioUtils.GetConfigCondition(projectTree, archName);

            writer.WriteStartElement("ItemDefinitionGroup");
            writer.WriteAttributeString("Condition", configCondition);

            if (module.Kind != EModuleKind.Custom)
            {
                BuildOptionsNode buildOptions = projectTree.GetMergedNode<BuildOptionsNode>(module.Node, true);
                BuildOutputNode buildOutput = projectTree.GetMergedNode<BuildOutputNode>(module.Node);
                CodegenNode codegen = projectTree.GetMergedNode<CodegenNode>(module.Node);
                DefinesNode defines = projectTree.GetMergedNode<DefinesNode>(module.Node, true);
                DialectNode dialect = projectTree.GetMergedNode<DialectNode>(module.Node);
                ExceptionsNode exceptions = projectTree.GetMergedNode<ExceptionsNode>(module.Node);
                ExternalNode external = projectTree.GetMergedNode<ExternalNode>(module.Node);
                FilesNode files = projectTree.GetMergedNode<FilesNode>(module.Node);
                FloatingPointNode floatingPoint = projectTree.GetMergedNode<FloatingPointNode>(module.Node);
                IncludeDirsNode includeDirs = projectTree.GetMergedNode<IncludeDirsNode>(module.Node, true);
                LinkOptionsNode linkOptions = projectTree.GetMergedNode<LinkOptionsNode>(module.Node);
                OptimizeNode optimize = projectTree.GetMergedNode<OptimizeNode>(module.Node);
                RuntimeNode runtime = projectTree.GetMergedNode<RuntimeNode>(module.Node);
                SymbolsNode symbols = projectTree.GetMergedNode<SymbolsNode>(module.Node);
                WarningsNode warnings = projectTree.GetMergedNode<WarningsNode>(module.Node);

                List<DependenciesNode> dependencies = [.. projectTree.GetNodes<DependenciesNode>(module.Node, module.IsBinary)];
                LibDirsNode libDirs = projectTree.GetMergedNode<LibDirsNode>(module.Node, true);

                bool isOptimizedBuild = VisualStudioUtils.IsOptimizedBuild(optimize);
                bool isDebugBuild = VisualStudioUtils.IsDebugBuild(optimize, symbols);
                bool hasAnyResourceFiles = files.Entries.Any(entry => entry.FileAction == EFileAction.Resource);

                writer.WriteStartElement("ClCompile");

                if (buildOptions.PchInclude is null)
                {
                    writer.WriteElementString("PrecompiledHeader", "NotUsing");
                }
                else
                {
                    writer.WriteElementString("PrecompiledHeader", "Use");
                    writer.WriteElementString("PrecompiledHeaderFile", buildOptions.PchInclude);
                }

                writer.WriteElementString("WarningLevel", VisualStudioUtils.GetWarningLevelString(warnings.WarningsLevel));
                writer.WriteElementBoolIfTrue("TreatWarningAsError", warnings.AreAllWarningsFatal);

                IEnumerable<string> disabledWarnings = warnings.Entries.Where((entry) => !entry.IsEnabled).Select((entry) => entry.WarningName);
                VisualStudioUtils.WriteArrayElement(writer, disabledWarnings, "DisableSpecificWarnings", "%(DisableSpecificWarnings)");

                IEnumerable<string> fatalWarnings = warnings.Entries.Where((entry) => entry.IsEnabled && entry.IsFatal).Select((entry) => entry.WarningName);
                VisualStudioUtils.WriteArrayElement(writer, fatalWarnings, "TreatSpecificWarningsAsErrors", "%(TreatSpecificWarningsAsErrors)");

                if (isOptimizedBuild && runtime.Runtime == ERuntime.Debug)
                {
                    writer.WriteElementString("BasicRuntimeChecks", "Default");
                }

                IEnumerable<string> defineEntryStrings = defines.Entries.Select((entry) => entry.DefineName);
                if (exceptions.ExceptionsMode == EExceptionsMode.Off)
                {
                    defineEntryStrings = defineEntryStrings.Concat(["_HAS_EXCEPTIONS=0"]);
                }
                VisualStudioUtils.WritePreprocessorDefinitions(writer, defineEntryStrings, false, configCondition);

                // TODO: Support for undefines?
                //IEnumerable<string> undefineEntryStrings = undefines.Entries.Select((entry) => entry.Define.Replace("\"", "\\\""));
                //WriteArrayElement(writer, undefineEntryStrings, "UndefinePreprocessorDefinitions", "%(UndefinePreprocessorDefinitions)");

                IEnumerable<string> includeDirsEntryStrings = includeDirs.Entries
                    .Where((entry) => !entry.IsExternal)
                    .Select((entry) => GetPath(entry.Path));
                VisualStudioUtils.WriteAdditionalIncludeDirs(writer, includeDirsEntryStrings);

                // TODO: Support for forced includes?
                //IEnumerable<string> forceIncludesEntryStrings = includeDirs.Entries.Select((entry) => GetPath(entry.Path));
                //WriteArrayElement(writer, forceIncludesEntryStrings, "ForcedIncludeFiles");

                // TODO: Support for using directories (C++/CLI, #using "")?
                //IEnumerable<string> usingDirsEntryStrings = usingDirs.Entries.Select((entry) => GetPath(entry.Path));
                //WriteArrayElement(writer, usingDirsEntryStrings, "AdditionalUsingDirectories", "%(AdditionalUsingDirectories)");

                // TODO: Support for forced usings (C++/CLI, #using "")?
                //IEnumerable<string> forceUsingsEntryStrings = includeDirs.Entries.Select((entry) => GetPath(entry.Path));
                //WriteArrayElement(writer, forceUsingsEntryStrings, "ForcedUsingFiles");

                if (symbols.SymbolsMode == ESymbolsMode.On)
                {
                    if (symbols.Embed)
                    {
                        writer.WriteElementString("DebugInformationFormat", "OldStyle");
                    }
                    else if (!toolset.EditAndContinue
                        || isOptimizedBuild
                        || buildOptions.ClrMode != EBuildClrMode.Off)
                    {
                        writer.WriteElementString("DebugInformationFormat", "ProgramDatabase");
                    }
                    else
                    {
                        writer.WriteElementString("DebugInformationFormat", "EditAndContinue");
                    }
                }
                else if (symbols.SymbolsMode == ESymbolsMode.Off)
                {
                    writer.WriteElementString("DebugInformationFormat", "None");
                }

                writer.WriteElementString("Optimization", optimize.OptimizationLevel switch
                {
                    EOptimizationLevel.Default => "Disabled",
                    EOptimizationLevel.Debug => "Disabled",
                    EOptimizationLevel.Off => "Disabled",
                    EOptimizationLevel.On => "Full",
                    EOptimizationLevel.Size => "MinSpace",
                    EOptimizationLevel.Speed => "MaxSpeed",
                    _ => throw new NotImplementedException($"Unsupported Optimization Level: {optimize.OptimizationLevel}"),
                });

                if (optimize.FunctionLevelLinking is not null)
                {
                    writer.WriteElementBool("FunctionLevelLinking", optimize.FunctionLevelLinking.Value);
                }
                else if (isOptimizedBuild)
                {
                    writer.WriteElementString("FunctionLevelLinking", "true");
                }

                if (optimize.Intrinsics is not null)
                {
                    writer.WriteElementBool("IntrinsicFunctions", optimize.Intrinsics.Value);
                }
                else if (isOptimizedBuild)
                {
                    writer.WriteElementString("IntrinsicFunctions", "true");
                }

                writer.WriteElementBoolIfFalse("SupportJustMyCode", optimize.JustMyCode);
                writer.WriteElementBoolIfTrue("OpenMPSupport", buildOptions.OpenMP);
                writer.WriteElementBoolIfTrue("OmitFramePointers", buildOptions.OmitFramePointers);

                if (isOptimizedBuild
                    || toolset.MultiProcess
                    || symbols.SymbolsMode == ESymbolsMode.On && symbols.Embed)
                {
                    writer.WriteElementString("MinimalRebuild", "false");
                }

                if (optimize.StringPooling is not null)
                {
                    writer.WriteElementBool("StringPooling", optimize.StringPooling.Value);
                }
                else if (isOptimizedBuild)
                {
                    writer.WriteElementString("StringPooling", "true");
                }

                writer.WriteElementString("RuntimeLibrary", runtime.Runtime switch
                {
                    ERuntime.Debug => runtime.StaticLink ? "MultiThreadedDebug" : "MultiThreadedDebugDLL",
                    ERuntime.Release => runtime.StaticLink ? "MultiThreaded" : "MultiThreadedDLL",
                    ERuntime.Default => runtime.StaticLink ? isDebugBuild ? "MultiThreadedDebug" : "MultiThreaded" : isDebugBuild ? "MultiThreadedDebugDLL" : "MultiThreadedDLL",
                    _ => throw new NotImplementedException($"Unsupported Runtime: {runtime.Runtime}"),
                });

                if (buildOptions.OmitDefaultLib)
                {
                    writer.WriteElementString("OmitDefaultLibName", "true");
                }

                if (exceptions.ExceptionsMode != EExceptionsMode.Default)
                {
                    writer.WriteElementString("ExceptionHandling", exceptions.ExceptionsMode switch
                    {
                        EExceptionsMode.On => "Sync",
                        EExceptionsMode.Off => "false",
                        EExceptionsMode.SEH => "Async",
                        _ => throw new NotImplementedException($"Unsupported Exceptions Mode: {exceptions.ExceptionsMode}"),
                    });
                }

                if (!buildOptions.RuntimeTypeInfo && buildOptions.ClrMode == EBuildClrMode.Off)
                {
                    writer.WriteElementString("RuntimeTypeInfo", "false");
                }
                else if (buildOptions.RuntimeTypeInfo)
                {
                    writer.WriteElementString("RuntimeTypeInfo", "true");
                }

                if (floatingPoint.Mode != EFloatingPointMode.Default)
                {
                    writer.WriteElementString("FloatingPointModel", floatingPoint.Mode switch
                    {
                        EFloatingPointMode.Fast => "Fast",
                        EFloatingPointMode.Precise => "Precise",
                        EFloatingPointMode.Strict => "Strict",
                        _ => throw new NotImplementedException($"Unsupported Floating Point Mode: {floatingPoint.Mode}"),
                    });
                }

                writer.WriteElementBool("FloatingPointExceptions", floatingPoint.AllowExceptions);

                writer.WriteElementString("InlineFunctionExpansion", optimize.InliningLevel switch
                {
                    EInliningLevel.Default => "Default",
                    EInliningLevel.Off => "Disabled",
                    EInliningLevel.Explicit => "OnlyExplicitInline",
                    EInliningLevel.On => "AnySuitable",
                    _ => throw new NotImplementedException($"Unsupported Inlining Level: {optimize.InliningLevel}"),
                });

                if (platform.Arch == EPlatformArch.X86)
                {
                    writer.WriteElementString("EnableEnhancedInstructionSet", codegen.CodegenMode switch
                    {
                        ECodegenMode.AVX => "AdvancedVectorExtensions",
                        ECodegenMode.AVX2 => "AdvancedVectorExtensions2",
                        ECodegenMode.AVX512 => "AdvancedVectorExtensions512",
                        ECodegenMode.SSE => "StreamingSIMDExtensions",
                        ECodegenMode.SSE2 => "StreamingSIMDExtensions2",
                        ECodegenMode.SSE3 => "StreamingSIMDExtensions2",
                        ECodegenMode.SSSE3 => "StreamingSIMDExtensions2",
                        ECodegenMode.SSE41 => "StreamingSIMDExtensions2",
                        ECodegenMode.SSE42 => "StreamingSIMDExtensions2",
                        _ => throw new NotImplementedException($"Unsupported instruction set for x86: {codegen.CodegenMode}"),
                    });
                }
                else if (platform.Arch == EPlatformArch.X86_64)
                {
                    writer.WriteElementString("EnableEnhancedInstructionSet", codegen.CodegenMode switch
                    {
                        ECodegenMode.AVX => "AdvancedVectorExtensions",
                        ECodegenMode.AVX2 => "AdvancedVectorExtensions2",
                        ECodegenMode.AVX512 => "AdvancedVectorExtensions512",
                        _ => throw new NotImplementedException($"Unsupported instruction set for x86_64: {codegen.CodegenMode}"),
                    });
                }
                else if (platform.Arch == EPlatformArch.Arm64)
                {
                    writer.WriteElementString("EnableEnhancedInstructionSet", codegen.CodegenMode switch
                    {
                        ECodegenMode.ARMv8_0 => "CPUExtensionRequirementsARMv80",
                        ECodegenMode.ARMv8_1 => "CPUExtensionRequirementsARMv81",
                        ECodegenMode.ARMv8_2 => "CPUExtensionRequirementsARMv82",
                        ECodegenMode.ARMv8_3 => "CPUExtensionRequirementsARMv83",
                        ECodegenMode.ARMv8_4 => "CPUExtensionRequirementsARMv84",
                        ECodegenMode.ARMv8_5 => "CPUExtensionRequirementsARMv85",
                        ECodegenMode.ARMv8_6 => "CPUExtensionRequirementsARMv86",
                        ECodegenMode.ARMv8_7 => "CPUExtensionRequirementsARMv87",
                        ECodegenMode.ARMv8_8 => "CPUExtensionRequirementsARMv88",
                        ECodegenMode.ARMv9_0 => "CPUExtensionRequirementsARMv90",
                        ECodegenMode.ARMv9_1 => "CPUExtensionRequirementsARMv90",
                        ECodegenMode.ARMv9_2 => "CPUExtensionRequirementsARMv90",
                        ECodegenMode.ARMv9_3 => "CPUExtensionRequirementsARMv90",
                        ECodegenMode.ARMv9_4 => "CPUExtensionRequirementsARMv90",
                        _ => throw new NotImplementedException($"Unsupported instruction set for ARM64: {codegen.CodegenMode}"),
                    });
                }

                if (toolset.MultiProcess)
                {
                    writer.WriteElementString("MultiProcessorCompilation", "true");
                }

                IEnumerable<string> buildOptionEntryStrings = buildOptions.Entries.Select((entry) => entry.OptionName);
                if (platform.Toolset == EToolset.Clang)
                {
                    // <OpenMPSupport> is ignored when using the clang toolset so we need to add it here
                    buildOptionEntryStrings = buildOptionEntryStrings.Concat(["/openmp"]);
                }
                IEnumerable<string> enabledWarnings = warnings.Entries.Where((entry) => entry.IsEnabled).Select((entry) => $"/w{entry.WarningName}");
                buildOptionEntryStrings = buildOptionEntryStrings.Concat(enabledWarnings);
                VisualStudioUtils.WriteArrayElement(writer, buildOptionEntryStrings, "AdditionalOptions", "%(AdditionalOptions)", " ");

                if (module.Node.HasValue("language"))
                {
                    writer.WriteElementString("CompileAs", module.Language switch
                    {
                        EModuleLanguage.C => "CompileAsC",
                        EModuleLanguage.Cpp => "CompileAsCpp",
                        _ => throw new NotImplementedException($"Unsupported Language: {module.Language}"),
                    });
                }

                // TODO: Support for calling convention?
                //if (buildOptions.CallingConvention is not null)
                //{
                //    writer.WriteElementString("CompileAs", buildOptions.CallingConvention switch
                //    {
                //        ECallingConvention.CDecl => "Cdecl",
                //        ECallingConvention.FastCall => "FastCall",
                //        ECallingConvention.StdCall => "StdCall",
                //        ECallingConvention.VectorCall => "VectorCall",
                //        _ => throw new NotImplementedException($"Unsupported Calling Convention: {buildOptions.CallingConvention}"),
                //    });
                //}

                if (dialect.CDialect != ECDialect.Default)
                {
                    writer.WriteElementString("LanguageStandard_C", dialect.CDialect switch
                    {
                        ECDialect.C11 => "stdc11",
                        ECDialect.C17 => "stdc17",
                        ECDialect.C23 => "stdclatest",
                        _ => throw new NotImplementedException($"Unsupported C Dialect: {dialect.CDialect}"),
                    });
                }

                if (dialect.CppDialect != ECppDialect.Default)
                {
                    writer.WriteElementString("LanguageStandard", dialect.CppDialect switch
                    {
                        ECppDialect.Cpp14 => "stdcpp14",
                        ECppDialect.Cpp17 => "stdcpp17",
                        ECppDialect.Cpp20 => "stdcpp20",
                        ECppDialect.Cpp23 => "stdcpplatest",
                        _ => throw new NotImplementedException($"Unsupported C++ Dialect: {dialect.CppDialect}"),
                    });
                }

                // TODO: Support for conformance mode? Right now doing this with /permissive- directly.
                //if (buildOptions.ConformanceMode is not null)
                //{
                //    writer.WriteElementBool("ConformanceMode", buildOptions.ConformanceMode.Value);
                //}

                // TODO: Support for struct member alignment?
                //if (buildOptions.StructMemberAlignment != EStructMemberAlignment.Default)
                //{
                //    writer.WriteElementString("StructMemberAlignment", buildOptions.StructMemberAlignment switch
                //    {
                //        EStructMemberAlignment.OneByte => "1Byte",
                //        EStructMemberAlignment.TwoBytes => "2Bytes",
                //        EStructMemberAlignment.FourBytes => "4Bytes",
                //        EStructMemberAlignment.EightBytes => "8Bytes",
                //        EStructMemberAlignment.SixteenBytes => "16Bytes",
                //        _ => throw new NotImplementedException($"Unsupported Struct Member Alignment: {buildOptions.StructMemberAlignment}"),
                //    });
                //}

                // TODO: Support for UseFullPaths?
                //writer.TryWriteElementBool("UseFullPaths", buildOptions.UseFullPaths);

                // TODO: Support for RemoveUnreferencedCodeData?
                //writer.TryWriteElementBool("RemoveUnreferencedCodeData", buildOptions.RemoveUnreferencedCodeData);

                // TODO: Support for CompileAsWinRT?
                //writer.TryWriteElementBool("CompileAsWinRT", buildOptions.CompileAsWinRT);

                if (external.WarningsLevel != EWarningsLevel.Default)
                {
                    writer.WriteElementString("ExternalWarningLevel", external.WarningsLevel switch
                    {
                        EWarningsLevel.All => "Level4",
                        EWarningsLevel.Extra => "Level4",
                        EWarningsLevel.On => "Level3",
                        EWarningsLevel.Off => "TurnOffAllWarnings",
                        _ => throw new NotImplementedException($"Unsupported External Warning Level: {external.WarningsLevel}"),
                    });
                }

                writer.WriteElementBool("TreatAngleIncludeAsExternal", external.AngleBrackets);
                //writer.WriteElementBool("UseStandardPreprocessor", true);

                if (module.Kind == EModuleKind.LibStatic)
                {
                    // TODO: Support for specifying the symbol file path for static libraries?
                    //writer.WriteElementString("ProgramDatabaseFile", "$(IntDir)$(TargetName).pdb");
                }

                writer.WriteEndElement();

                BuildEventNode buildEvent = projectTree.GetMergedNode<BuildEventNode>(module.Node, (n) => n.EventName == EBuildEvent.Build, false);
                List<CommandNode> buildCommands = [.. projectTree.GetNodes<CommandNode>(buildEvent.Node)];
                if (buildCommands.Count != 0)
                {
                    writer.WriteStartElement("CustomBuildStep");

                    if (!string.IsNullOrEmpty(buildEvent.EventMessage))
                    {
                        writer.WriteElementString("Message", buildEvent.EventMessage);
                    }

                    IEnumerable<string> buildCommandStrings = buildCommands.Select((entry) => entry.GetCommandString());
                    VisualStudioUtils.WriteArrayElement(writer, buildCommandStrings, "Command", null, "\r\n");

                    OutputsNode buildEventOutputs = projectTree.GetMergedNode<OutputsNode>(buildEvent.Node, false);
                    IEnumerable<string> buildEventOutputsStrings = buildEventOutputs.Entries.Select((entry) => GetPath(entry.FilePath));
                    VisualStudioUtils.WriteArrayElement(writer, buildEventOutputsStrings, "Outputs");

                    InputsNode buildEventInputs = projectTree.GetMergedNode<InputsNode>(buildEvent.Node, false);
                    IEnumerable<string> buildEventInputsStrings = buildEventInputs.Entries.Select((entry) => GetPath(entry.FilePath));
                    VisualStudioUtils.WriteArrayElement(writer, buildEventInputsStrings, "Inputs");

                    writer.WriteEndElement();
                }

                // TODO: FX compiler support?

                if (hasAnyResourceFiles)
                {
                    writer.WriteStartElement("ResourceCompile");
                    VisualStudioUtils.WritePreprocessorDefinitions(writer, defineEntryStrings, true, configCondition);
                    VisualStudioUtils.WriteAdditionalIncludeDirs(writer, includeDirsEntryStrings);
                    // TODO: Culture support?
                    writer.WriteEndElement();
                }

                writer.WriteStartElement("Link");

                writer.WriteElementString("SubSystem", module.Kind == EModuleKind.AppConsole ? "Console" : "Windows");
                // TODO: Difficult to understand what this really controls...
                //writer.WriteElementBoolIfTrue("FullProgramDatabaseFile", symbols.SymbolsMode == ESymbolsMode.On);
                if (symbols.SymbolsMode != ESymbolsMode.Default)
                {
                    writer.WriteElementString("GenerateDebugInformation", symbols.SymbolsMode switch
                    {
                        ESymbolsMode.On => "DebugFull",
                        ESymbolsMode.Off => "false",
                        _ => throw new NotImplementedException($"Unsupported Symbol Mode: {symbols.SymbolsMode}"),
                    });
                }

                writer.WriteElementBoolIfTrue("EnableCOMDATFolding", isOptimizedBuild);
                writer.WriteElementBoolIfTrue("OptimizeReferences", isOptimizedBuild);

                if (optimize.LinkTimeOptimizationLevel == ELinkTimeOptimizationLevel.On)
                {
                    writer.WriteElementString("LinkTimeCodeGeneration", "UseLinkTimeCodeGeneration");
                }

                if (module.IsBinary)
                {
                    IEnumerable<string> linkDeps = dependencies
                        .SelectMany((entry) => entry.Entries)
                        .Where((entry) => entry.Kind == EDependencyKind.File || entry.Kind == EDependencyKind.System)
                        .Select((entry) => VisualStudioUtils.EnsureLibraryExtension(entry.DependencyName));
                    VisualStudioUtils.WriteArrayElement(writer, linkDeps, "AdditionalDependencies", "%(AdditionalDependencies)");
                }

                IEnumerable<string> libPaths = libDirs.Entries.Where((entry) => !entry.IsSystem).Select((entry) => GetPath(entry.Path));
                VisualStudioUtils.WriteArrayElement(writer, libPaths, "AdditionalLibraryDirectories", "%(AdditionalLibraryDirectories)");

                writer.WriteElementBoolIfTrue("TreatLinkerWarningAsErrors", warnings.AreAllWarningsFatal);

                // TODO: support for individual fatal link errors by adding `/wx:a,b,c` automatically?
                IEnumerable<string> linkerOptions = linkOptions.Entries.Select((entry) => entry.OptionName);
                VisualStudioUtils.WriteArrayElement(writer, linkerOptions, "AdditionalOptions", "%(AdditionalOptions)", " ");

                if (module.Kind != EModuleKind.LibStatic)
                {
                    if (module.Kind == EModuleKind.LibShared)
                    {
                        string libDir = module.GetLibDir(buildOutput);
                        string importLib = Path.Join(libDir, module.TargetName + ".lib");
                        writer.WriteElementString("ImportLibrary", importLib);
                    }

                    writer.TryWriteElementString("EntryPointSymbol", module.EntryPoint);
                    writer.WriteElementBoolIfTrue("GenerateMapFile", module.MakeMapFile);

                    // TODO: C++ module definition support (.def)?
                    //writer.WriteElementString("ModuleDefinitionFile", ...);

                    // TODO: Support for ignoring specific default libraries?
                    //IEnumerable<string> ignoredDefaultLibs = linkOptions.IgnoredDefaultLibraries.Select((entry) => VisualStudioUtils.EnsureLibraryExtension(entry));
                    //WriteArrayElement(writer, ignoredDefaultLibs, "IgnoreSpecificDefaultLibraries");

                    // TODO: Support for large address aware?
                    //writer.WriteElementBoolIfTrue("LargeAddressAware", buildOptions.LargeAddressAware);

                    // TODO: Support for explicit PDB file path?
                    //if (symbols.SymbolsMode != ESymbolsMode.Off && !symbols.Embed)
                    //{
                    //    writer.WriteElementString("ProgramDatabaseFile", GetPath(symbols.Path));
                    //}
                }

                writer.WriteEndElement();

                if (module.Kind == EModuleKind.LibStatic)
                {
                    writer.WriteStartElement("Lib");
                    writer.WriteElementBoolIfTrue("TreatLibWarningAsErrors", warnings.AreAllWarningsFatal);

                    // If we have resource files we need to specify the TargetMachine explicitly.
                    // See: https://learn.microsoft.com/en-us/cpp/build/reference/machine-specify-target-platform?view=msvc-170
                    if (module.Kind == EModuleKind.LibStatic && hasAnyResourceFiles)
                    {
                        writer.WriteElementString("TargetMachine", platform.Arch switch
                        {
                            EPlatformArch.X86 => "MachineX86",
                            EPlatformArch.X86_64 => "MachineX64",
                            EPlatformArch.Arm => "MachineARM",
                            EPlatformArch.Arm64 => "MachineARM64",
                            _ => throw new NotImplementedException($"Unsupported Target Machine: {platform.Arch}"),
                        });
                    }

                    writer.WriteEndElement();
                }
                else
                {
                    writer.WriteStartElement("Manifest");

                    writer.WriteElementString("EnableDpiAwareness", buildOptions.DpiAwarenessMode switch
                    {
                        EDpiAwareMode.None => "false",
                        EDpiAwareMode.HighDpiAware => "true",
                        EDpiAwareMode.PerMonitorHighDpiAware => "PerMonitorHighDPIAware",
                        _ => throw new NotImplementedException($"Unsupported DPI Awareness Mode: {buildOptions.DpiAwarenessMode}"),
                    });

                    IEnumerable<string> extraManifestFiles = files.Entries.Where((entry) => entry.FileAction == EFileAction.Manifest).Select((entry) => GetPath(entry.FilePath));
                    VisualStudioUtils.WriteArrayElement(writer, extraManifestFiles, "AdditionalManifestFiles", "%(AdditionalManifestFiles)");

                    writer.WriteEndElement();
                }
            }

            void writeBuildEvent(EBuildEvent evt)
            {
                BuildEventNode buildEvent = projectTree.GetMergedNode<BuildEventNode>(module.Node, (n) => n.EventName == evt, false);
                List<CommandNode> buildCommands = [.. projectTree.GetNodes<CommandNode>(buildEvent.Node)];
                if (buildCommands.Count != 0)
                {
                    writer.WriteStartElement(evt switch
                    {
                        EBuildEvent.Prebuild => "PreBuildEvent",
                        //EBuildEvent.Build => "BuildEvent",
                        EBuildEvent.Postbuild => "PostBuildEvent",
                        EBuildEvent.Prelink => "PreLinkEvent",
                        //EBuildEvent.Postlink => "PostLinkEvent",
                        //EBuildEvent.Clean => "CleanEvent",
                        _ => throw new NotImplementedException($"Unsupported build event: {evt}"),
                    });

                    if (!string.IsNullOrEmpty(buildEvent.EventMessage))
                    {
                        writer.WriteElementString("Message", buildEvent.EventMessage);
                    }

                    IEnumerable<string> buildCommandStrings = buildCommands.Select((entry) => entry.GetCommandString());
                    VisualStudioUtils.WriteArrayElement(writer, buildCommandStrings, "Command", null, "\r\n");

                    writer.WriteEndElement();
                }
            }
            writeBuildEvent(EBuildEvent.Prebuild);
            writeBuildEvent(EBuildEvent.Prelink);
            writeBuildEvent(EBuildEvent.Postbuild);

            if (toolset.Path is not null)
            {
                writer.WriteStartElement("BuildLog");
                writer.WriteElementString("Path", toolset.Path);
                writer.WriteEndElement();
            }

            writer.WriteEndElement();
        }
    }

    private void WriteFiles(XmlWriter writer)
    {
        foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
        {
            fileGroup.WriteFiles(writer);
        }
    }

    private static void WriteLanguageTargetImports(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
        writer.WriteEndElement();
    }

    private void WriteExtensionSettings(XmlWriter writer)
    {
        writer.WriteStartElement("ImportGroup");
        writer.WriteAttributeString("Label", "ExtensionSettings");

        foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
        {
            fileGroup.WriteExtensionSettings(writer);
        }

        writer.WriteEndElement();
    }

    private void WriteExtensionTargetImports(XmlWriter writer)
    {
        writer.WriteStartElement("ImportGroup");
        writer.WriteAttributeString("Label", "ExtensionTargets");

        foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
        {
            fileGroup.WriteExtensionTargets(writer);
        }

        writer.WriteEndElement();
    }

    private void WriteProjectReferences(XmlWriter writer)
    {
        writer.WriteStartElement("ItemGroup");

        Dictionary<string, (ModuleNode Module, bool UseManagedMetadata)> references = [];

        foreach ((ResolvedProjectTree projectTree, ModuleNode module, _) in EnumerateModuleConfigs())
        {
            if (!module.IsApp)
            {
                continue;
            }

            BuildOptionsNode buildOptions = projectTree.GetMergedNode<BuildOptionsNode>(module.Node);

            bool isManaged = VisualStudioUtils.IsManaged(module, buildOptions);
            bool isClrMixed = VisualStudioUtils.IsClrMixed(module, buildOptions);
            List<DependenciesNode> dependencies = [.. projectTree.GetNodes<DependenciesNode>(module.Node, module.IsBinary)];

            foreach (DependenciesEntryNode depEntry in dependencies.SelectMany((entry) => entry.Entries))
            {
                if (depEntry.Kind != EDependencyKind.Default && depEntry.Kind != EDependencyKind.Link)
                {
                    continue;
                }

                if (!projectTree.IndexedNodes.TryGetNode(depEntry.DependencyName, out ModuleNode? depModule))
                {
                    throw new InvalidOperationException($"No module found with name '{depEntry.DependencyName}', but module '{module.ModuleName}' depends on it.");
                }

                if (!ShouldWriteProjectReference(projectTree, depModule))
                {
                    continue;
                }

                bool useManagedMetadata = (isManaged || isClrMixed) && depModule.Kind != EModuleKind.LibStatic;
                if (references.TryGetValue(depModule.ModuleName, out (ModuleNode Module, bool UseManagedMetadata) existingRef))
                {
                    references[depModule.ModuleName] = (existingRef.Module, existingRef.UseManagedMetadata || useManagedMetadata);
                }
                else
                {
                    references[depModule.ModuleName] = (depModule, useManagedMetadata);
                }
            }
        }

        foreach ((ModuleNode depModule, bool useManagedMetadata) in references.Values)
        {
            writer.WriteStartElement("ProjectReference");
            writer.WriteAttributeString("Include", $"{depModule.ModuleName}{ProjectExtension}");

            writer.WriteElementString("Project", ModuleGroupTree.GetModuleGuid(depModule));

            if (useManagedMetadata)
            {
                writer.WriteElementString("Private", "true");
                writer.WriteElementString("ReferenceOutputAssembly", "true");
                writer.WriteElementString("CopyLocalSatelliteAssemblies", "false");
                writer.WriteElementString("LinkLibraryDependencies", "true");
                writer.WriteElementString("UseLibraryDependencyInputs", "false");
            }

            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }

    private static bool ShouldWriteProjectReference(ResolvedProjectTree projectTree, ModuleNode module)
    {
        if (module.Kind != EModuleKind.LibStatic && module.Kind != EModuleKind.LibShared)
        {
            return false;
        }

        if (!string.IsNullOrEmpty(module.ProjectFile))
        {
            return true;
        }

        FilesNode files = projectTree.GetMergedNode<FilesNode>(module.Node);
        return files.Entries.Any((entry) => !entry.IsExcludedFromBuild && entry.FileAction == EFileAction.Build);
    }

    private void AddGeneratedFilesForCustomBuildRule(ResolvedProjectTree projectTree, ModuleNode module, BuildRuleNode buildRule, string sourceFilePath)
    {
        List<CommandNode> commands = [.. projectTree.GetNodes<CommandNode>(buildRule.Node)];
        if (commands.Count == 0)
        {
            return;
        }

        OutputsNode outputs = projectTree.GetMergedNode<OutputsNode>(buildRule.Node, false);
        IEnumerable<string> outputFilePaths = outputs.Entries.Select((entry) => entry.FilePath);

        foreach (string outputFilePath in outputFilePaths)
        {
            EFileAction fileAction = FilesEntryNode.GetDefaultFileAction(outputFilePath);
            EFileBuildRule fileBuildRule = FilesEntryNode.GetDefaultFileBuildRule(outputFilePath);

            foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
            {
                if (fileGroup.CanHandleFile(outputFilePath, fileAction, fileBuildRule))
                {
                    fileGroup.AddGeneratedFile(projectTree, module, outputFilePath, sourceFilePath, fileAction, fileBuildRule);
                    break;
                }
            }
        }
    }

    private void CreateFileGroups()
    {
        _fileGroups = VisualStudioUtils.CreateFileGroups(_projectService, _outputPath);

        foreach ((ResolvedProjectTree projectTree, string _) in VisualStudioUtils.EnumerateConfigs(_projectService))
        {
            if (!projectTree.IndexedNodes.TryGetNode(_moduleName, out ModuleNode? module))
            {
                continue;
            }

            FilesNode files = projectTree.GetMergedNode<FilesNode>(module.Node);

            foreach (FilesEntryNode entry in files.Entries)
            {
                // Search for a file group that can handle this file entry
                bool foundGroup = false;
                foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
                {
                    if (fileGroup.CanHandleFile(entry))
                    {
                        fileGroup.AddFile(projectTree, module, entry);
                        foundGroup = true;
                        break;
                    }
                }

                // This should never happen because the NoneFileGroup can handle any file
                if (!foundGroup)
                {
                    throw new Exception("No file group found for file: " + entry.FilePath);
                }

                // Add each output file from custom build rules
                if (!entry.IsExcludedFromBuild
                    && entry.FileAction == EFileAction.Build
                    && entry.FileBuildRule == EFileBuildRule.Custom)
                {
                    BuildRuleNode buildRule = projectTree.GetMergedNode<BuildRuleNode>(module.Node, (n) => n.RuleName == entry.BuildRuleName, false);
                    if (!buildRule.HasValue(0) || buildRule.RuleName != entry.BuildRuleName)
                    {
                        throw new Exception($"No build rule with the name '{entry.BuildRuleName}' was found.");
                    }

                    if (buildRule.LinkOutput)
                    {
                        AddGeneratedFilesForCustomBuildRule(projectTree, module, buildRule, entry.FilePath);
                    }
                }
            }
        }

        foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
        {
            fileGroup.SortFiles();
            fileGroup.SetupVirtualPaths();
        }
    }
}
