// Copyright Chad Engler

using Harvest.Make.Extensions;
using Harvest.Make.Projects.Nodes;
using System.CommandLine.Invocation;
using System.Text;
using System.Xml;

namespace Harvest.Make.Projects.Generators.vs2022;

internal class VcxprojGenerator(IProjectService projectService, ProjectGeneratorHelper helper)
{
    public const string ProjectExtension = ".vcxproj";
    public const string FiltersExtension = ".vcxproj.filters";
    public const string XmlNamespace = "http://schemas.microsoft.com/developer/msbuild/2003";

    private readonly IProjectService _projectService = projectService;
    private readonly ProjectGeneratorHelper _helper = helper;
    private string _outputPath = "";
    private List<IVisualStudioFileGroup> _fileGroups = [];

    public void Generate(InvocationContext context, ModuleNode module)
    {
        CreateFileGroups(context, module);

        GenerateProjectFile(context, module);
        GenerateFiltersFile(module);
        GenerateUserFile(module);
    }

    private void GenerateProjectFile(InvocationContext context, ModuleNode module)
    {
        _outputPath = Path.Join(_helper.BuildOutput.ProjectDir, $"{module.ModuleName}{ProjectExtension}");

        using FileStream stream = new(_outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
        using XmlWriter writer = XmlWriter.Create(stream, new XmlWriterSettings { Encoding = Encoding.UTF8, Indent = true });

        writer.WriteStartDocument();
        writer.WriteStartElement("Project", XmlNamespace);
        writer.WriteAttributeString("DefaultTargets", "Build");

        WriteConfigurations(writer);
        WriteGlobals(writer, context, module);
        WriteImportDefaultProperties(writer);
        WriteConfigurationProperties(writer, context, module);
        WriteImportProperties(writer);
        WriteUserMacros(writer);
        WriteOutputProperties(writer, context, module);
        WriteItemDefinitionGroups(writer, context, module);
        // TODO: managed assembly references (<ItemGroup><Reference Include="..." /></ItemGroup>)
        WriteFiles(writer);
        WriteProjectReferences(writer, context, module);
        WriteLanguageTargetImports(writer);
        WriteExtensionTargetImports(writer);
        // TODO: nuget references for Visual Studio to restore, is this needed?
        // Right now hemake downloads the nuget packages to .build/ & sets up refs.

        writer.WriteEndElement();
        writer.WriteEndDocument();
    }

    private void GenerateFiltersFile(ModuleNode module)
    {
        _outputPath = Path.Join(_helper.BuildOutput.ProjectDir, $"{module.ModuleName}{ProjectExtension}");

        using FileStream stream = new(_outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
        using XmlWriter writer = XmlWriter.Create(stream, new XmlWriterSettings { Encoding = Encoding.UTF8, Indent = true });

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

    private void GenerateUserFile(ModuleNode module)
    {
        // TODO: debugger settings: command, args, flavor, type, working dir, environment variables, etc.

        //_outputPath = Path.Join(_helper.BuildOutput.ProjectDir, $"{module.ModuleName}{ProjectExtension}");

        //using FileStream stream = new(_outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
        //using XmlWriter writer = XmlWriter.Create(stream, new XmlWriterSettings { Encoding = Encoding.UTF8, Indent = true });

        //writer.WriteStartDocument();
        //writer.WriteStartElement("Project", XmlNamespace);
        //writer.WriteAttributeString("ToolsVersion", "Current"); // vs2022 user version

        //writer.WriteStartElement("PropertyGroup");
        //writer.WriteEndElement();

        //writer.WriteEndElement();
        //writer.WriteEndDocument();
    }

    private string GetPath(string path)
    {
        return VisualStudioUtils.TranslatePath(_outputPath, path);
    }

    private void WriteConfigurations(XmlWriter writer)
    {
        writer.WriteStartElement("ItemGroup");
        writer.WriteAttributeString("Label", "ProjectConfigurations");

        VisualStudioUtils.ForEachConfig(_helper, (configuration, platform, archName) =>
        {
            writer.WriteStartElement("ProjectConfiguration");
            writer.WriteAttributeString("Include", VisualStudioUtils.GetConfigName(configuration, platform, archName));
            writer.WriteElementString("Configuration", VisualStudioUtils.GetConfigName(configuration, platform));
            writer.WriteElementString("Platform", archName);
            writer.WriteEndElement();
        });

        writer.WriteEndElement();
    }

    private void WriteGlobals(XmlWriter writer, InvocationContext invocationContext, ModuleNode module)
    {
        bool isWindows = _helper.Platforms.Where((n) => n.System == EPlatformSystem.Windows).Any();
        ProjectContext context = _projectService.CreateProjectContext(invocationContext, module);

        BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);
        SystemNode dotnetSystemNode = _projectService.GetMergedNode<SystemNode>(context, module, (n) => n.System == EPlatformSystem.DotNet);
        ToolsetNode toolsetNode = _projectService.GetMergedNode<ToolsetNode>(context, module, (n) => n.Toolset == EToolset.MSVC);
        SystemNode windowsSystemNode = _projectService.GetMergedNode<SystemNode>(context, module, (n) => n.System == EPlatformSystem.Windows);

        writer.WriteStartElement("PropertyGroup");
        writer.WriteAttributeString("Label", "Globals");

        // TODO: Is this needed?
        // I think we'd need this if we let users specify an explicit virtual path for files because
        // that might result in multiple files of the same name in the same virtual dir?
        //writer.WriteElementString("IgnoreWarnCompileDuplicatedFilename", "true");

        if (isWindows)
        {
            bool isManaged = VisualStudioUtils.IsManaged(module, buildOptions);
            bool isClrMixed = VisualStudioUtils.IsClrMixed(module, buildOptions);

            if (isManaged || isClrMixed)
            {
                writer.WriteElementString("TargetFramework", dotnetSystemNode.Version);
            }

            if (isManaged)
            {
                writer.WriteElementString("Keyword", "ManagedCProj");
            }
            else
            {
                writer.WriteElementString("Keyword", "Win32Proj");
            }
            writer.WriteElementString("RootNamespace", module.Name);
        }


        writer.WriteElementString("ProjectGuid", ModuleGroupTree.GetModuleGuid(module));
        writer.WriteElementString("ProjectName", module.Name);

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

        writer.WriteElementString("WindowsTargetPlatformVersion", windowsSystemNode.Version);
        writer.WriteElementString("DisableFastUpToDateCheck", toolsetNode.FastUpToDateCheck.ToString());

        writer.WriteEndElement();
    }

    private static void WriteImportDefaultProperties(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
        writer.WriteEndElement();
    }

    private void WriteConfigurationProperties(XmlWriter writer, InvocationContext invocationContext, ModuleNode module)
    {
        VisualStudioUtils.ForEachConfig(_helper, (configuration, platform, archName) =>
        {
            ProjectContext context = _projectService.CreateProjectContext(invocationContext, module, configuration, platform);
            BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);
            ToolsetNode toolsetNode = _projectService.GetMergedNode<ToolsetNode>(context, module, (n) => n.Toolset == platform.Toolset);
            string configCondition = VisualStudioUtils.GetConfigCondition(configuration, platform, archName);

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
                    writer.WriteElementString("VCToolsVersion", toolsetNode.Version);
                }
                writer.WriteElementString("PlatformToolset", "v143"); // VS 2022
            }

            if (module.Kind != EModuleKind.Custom && module.Kind != EModuleKind.Content)
            {
                RuntimeNode runtimeNode = _projectService.GetMergedNode<RuntimeNode>(context, module);
                if (runtimeNode.Runtime == ERuntime.Debug || (runtimeNode.Runtime == ERuntime.Default && configuration.ConfigName == "Debug"))
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
                else if (buildOptions.MfcMode == EBuildMfcMode.Static)
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
                        // Always enabled for csharp code
                        if (module.Language == EModuleLanguage.CSharp)
                        {
                            writer.WriteElementString("CLRSupport", "true");
                        }
                        break;
                    case EBuildClrMode.NetCore:
                        writer.WriteElementString("CLRSupport", "NetCore");
                        break;
                }

                SanitizeNode sanitize = _projectService.GetMergedNode<SanitizeNode>(context, module);
                if (sanitize.EnableAddress)
                {
                    writer.WriteElementString("EnableASAN", "true");
                }
                if (sanitize.EnableFuzzer)
                {
                    writer.WriteElementString("EnableFuzzer", "true");
                }

                OptimizeNode optimize = _projectService.GetMergedNode<OptimizeNode>(context, module);
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
                BuildOutputNode buildOutput = _projectService.GetMergedNode<BuildOutputNode>(context, module);
                string outDir = GetPath(buildOutput.GetTargetDir(module.Kind));
                writer.WriteElementString("OutDir", $"{outDir}\\");

                string objDir = GetPath(buildOutput.ObjDir);
                writer.WriteElementString("IntDir", $"{objDir}\\");
            }

            writer.WriteEndElement();
        });
    }

    private void WriteImportProperties(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
        writer.WriteEndElement();

        WriteExtensionSettings(writer);

        VisualStudioUtils.ForEachConfig(_helper, (configuration, platform, archName) =>
        {
            string configCondition = VisualStudioUtils.GetConfigCondition(configuration, platform, archName);

            writer.WriteStartElement("ImportGroup");
            writer.WriteAttributeString("Label", "PropertySheets");
            writer.WriteAttributeString("Condition", configCondition);

            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
            writer.WriteAttributeString("Condition", $"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
            writer.WriteAttributeString("Label", "LocalAppDataPlatform");
            writer.WriteEndElement();

            writer.WriteEndElement();
        });
    }

    private static void WriteUserMacros(XmlWriter writer)
    {
        writer.WriteStartElement("PropertyGroup");
        writer.WriteAttributeString("Label", "UserMacros");
        writer.WriteEndElement();
    }

    private void WriteOutputProperties(XmlWriter writer, InvocationContext invocationContext, ModuleNode module)
    {
        VisualStudioUtils.ForEachConfig(_helper, (configuration, platform, archName) =>
        {
            ProjectContext context = _projectService.CreateProjectContext(invocationContext, module, configuration, platform);
            BuildOutputNode buildOutput = _projectService.GetMergedNode<BuildOutputNode>(context, module);
            BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);
            LinkOptionsNode linkOptions = _projectService.GetMergedNode<LinkOptionsNode>(context, module);
            OptimizeNode optimize = _projectService.GetMergedNode<OptimizeNode>(context, module);
            string configCondition = VisualStudioUtils.GetConfigCondition(configuration, platform, archName);

            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", configCondition);

            string outDir = GetPath(buildOutput.GetTargetDir(module.Kind));
            writer.WriteElementString("OutDir", $"{outDir}\\");

            string objDir = GetPath(buildOutput.ObjDir);
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
                    writer.WriteElementBool("IgnoreImportLibrary", buildOutput.MakeImportLib);
                }

                writer.WriteElementString("TargetName", buildOutput.TargetName ?? module.ModuleName);

                if (buildOutput.TargetExtension is not null)
                {
                    writer.WriteElementString("TargetExt", buildOutput.TargetExtension);
                }
                else
                {
                    writer.WriteStartElement("TargetExt");
                    writer.WriteEndElement();
                }

                IncludeDirsNode includeDirs = _projectService.GetMergedNode<IncludeDirsNode>(context, module);
                IEnumerable<string> externalIncludePaths = includeDirs.Entries.Where((entry) => entry.IsExternal).Select((entry) => GetPath(entry.Path));
                VisualStudioUtils.WriteArrayElement(writer, externalIncludePaths, "ExternalIncludePath", "%(ExternalIncludePath)");

                LibDirsNode libDirs = _projectService.GetMergedNode<LibDirsNode>(context, module);
                IEnumerable<string> systemLibPaths = libDirs.Entries.Where((entry) => entry.IsSystem).Select((entry) => GetPath(entry.Path));
                VisualStudioUtils.WriteArrayElement(writer, systemLibPaths, "LibraryPath", "%(LibraryPath)");

                if (!buildOutput.MakeExeManifest)
                {
                    writer.WriteElementString("GenerateManifest", "false");
                }

                writer.WriteElementBool("EnableClangTidyCodeAnalysis", buildOptions.RunClangTidy);
                writer.WriteElementBool("RunCodeAnalysis", buildOptions.RunCodeAnalysis);
            }

            writer.WriteEndElement();
        });
    }

    private void WriteItemDefinitionGroups(XmlWriter writer, InvocationContext invocationContext, ModuleNode module)
    {
        VisualStudioUtils.ForEachConfig(_helper, (configuration, platform, archName) =>
        {
            ProjectContext context = _projectService.CreateProjectContext(invocationContext, module, configuration, platform);
            ToolsetNode toolset = _projectService.GetMergedNode<ToolsetNode>(context, module, (n) => n.Toolset == platform.Toolset);
            string configCondition = VisualStudioUtils.GetConfigCondition(configuration, platform, archName);

            writer.WriteStartElement("ItemDefinitionGroup");
            writer.WriteAttributeString("Condition", configCondition);

            if (module.Kind != EModuleKind.Custom)
            {
                BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);
                BuildOutputNode buildOutput = _projectService.GetMergedNode<BuildOutputNode>(context, module);
                CodegenNode codegen = _projectService.GetMergedNode<CodegenNode>(context, module);
                DefinesNode defines = _projectService.GetMergedNode<DefinesNode>(context, module);
                DialectNode dialect = _projectService.GetMergedNode<DialectNode>(context, module);
                ExceptionsNode exceptions = _projectService.GetMergedNode<ExceptionsNode>(context, module);
                ExternalNode external = _projectService.GetMergedNode<ExternalNode>(context, module);
                FilesNode files = _projectService.GetMergedNode<FilesNode>(context, module);
                FloatingPointNode floatingPoint = _projectService.GetMergedNode<FloatingPointNode>(context, module);
                IncludeDirsNode includeDirs = _projectService.GetMergedNode<IncludeDirsNode>(context, module);
                LinkOptionsNode linkOptions = _projectService.GetMergedNode<LinkOptionsNode>(context, module);
                OptimizeNode optimize = _projectService.GetMergedNode<OptimizeNode>(context, module);
                RuntimeNode runtime = _projectService.GetMergedNode<RuntimeNode>(context, module);
                SymbolsNode symbols = _projectService.GetMergedNode<SymbolsNode>(context, module);
                WarningsNode warnings = _projectService.GetMergedNode<WarningsNode>(context, module);

                List<DependenciesNode> dependencies = _projectService.GetNodes<DependenciesNode>(context, module);
                LibDirsNode libDirs = _projectService.GetMergedNode<LibDirsNode>(context, module);

                bool isOptimizedBuild = VisualStudioUtils.IsOptimizedBuild(optimize);
                bool isDebugBuild = VisualStudioUtils.IsDebugBuild(optimize, symbols);
                bool hasAnyResourceFiles = files.Entries.Any(entry => entry.ResolvedFileAction == EFileAction.Resource);

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

                IEnumerable<string> defineEntryStrings = defines.Entries.Select((entry) => entry.Define);
                if (exceptions.ExceptionsMode == EExceptionsMode.Off)
                {
                    defineEntryStrings = defineEntryStrings.Concat(["_HAS_EXCEPTIONS=0"]);
                }
                VisualStudioUtils.WritePreprocessorDefinitions(writer, defineEntryStrings, false, VisualStudioUtils.GetConfigCondition(configuration, platform, archName));

                // TODO: Support for undefines?
                //IEnumerable<string> undefineEntryStrings = undefines.Entries.Select((entry) => entry.Define.Replace("\"", "\\\""));
                //WriteArrayElement(writer, undefineEntryStrings, "UndefinePreprocessorDefinitions", "%(UndefinePreprocessorDefinitions)");

                IEnumerable<string> includeDirsEntryStrings = includeDirs.Entries.Select((entry) => GetPath(entry.Path));
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

                writer.WriteElementBool("SupportJustMyCode", optimize.JustMyCode);
                writer.WriteElementBool("IntrinsicFunctions", buildOptions.OpenMP);
                writer.WriteElementBool("OmitFramePointers", buildOptions.OmitFramePointers);

                if (isOptimizedBuild
                    || toolset.MultiProcess
                    || (symbols.SymbolsMode == ESymbolsMode.On && symbols.Embed))
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
                    ERuntime.Default => runtime.StaticLink ? (isDebugBuild ? "MultiThreadedDebug" : "MultiThreaded") : (isDebugBuild ? "MultiThreadedDebugDLL" : "MultiThreadedDLL"),
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

                IEnumerable<string> buildOptionEntryStrings = buildOptions.Entries.Select((entry) => entry.Option);
                if (platform.Toolset == EToolset.Clang)
                {
                    // <OpenMPSupport> is ignored when using the clang toolset so we need to add it here
                    buildOptionEntryStrings = buildOptionEntryStrings.Concat(["/openmp"]);
                }
                IEnumerable<string> enabledWarnings = warnings.Entries.Where((entry) => entry.IsEnabled).Select((entry) => $"/w{entry.WarningName}");
                buildOptionEntryStrings = buildOptionEntryStrings.Concat(enabledWarnings);
                VisualStudioUtils.WriteArrayElement(writer, buildOptionEntryStrings, "AdditionalOptions", "%(AdditionalOptions)", " ");

                writer.WriteElementString("CompileAs", module.Language switch
                {
                    EModuleLanguage.C => "CompileAsC",
                    EModuleLanguage.Cpp => "CompileAsCpp",
                    _ => throw new NotImplementedException($"Unsupported Language: {module.Language}"),
                });

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
                writer.WriteElementBool("UseStandardPreprocessor", true);

                if (module.Kind == EModuleKind.LibStatic)
                {
                    // TODO: Support for specifying the symbol file path for static libraries?
                    //writer.WriteElementString("ProgramDatabaseFile", "$(IntDir)$(TargetName).pdb");
                }

                writer.WriteEndElement();

                BuildEventNode buildEvent = _projectService.GetMergedNode<BuildEventNode>(context, module, (n) => n.EventName == EBuildEvent.Build, false);
                List<CommandNode> buildCommands = _projectService.GetNodes<CommandNode>(context, buildEvent, false);
                if (buildCommands.Count != 0)
                {
                    writer.WriteStartElement("CustomBuildStep");

                    if (!string.IsNullOrEmpty(buildEvent.Message))
                    {
                        writer.WriteElementString("Message", buildEvent.Message);
                    }

                    IEnumerable<string> buildCommandStrings = buildCommands.Select((entry) => entry.GetCommandString());
                    VisualStudioUtils.WriteArrayElement(writer, buildCommandStrings, "Command", null, "\r\n");

                    OutputsNode buildEventOutputs = _projectService.GetMergedNode<OutputsNode>(context, buildEvent, false);
                    IEnumerable<string> buildEventOutputsStrings = buildEventOutputs.Entries.Select((entry) => GetPath(entry.FilePath));
                    VisualStudioUtils.WriteArrayElement(writer, buildEventOutputsStrings, "Outputs");

                    InputsNode buildEventInputs = _projectService.GetMergedNode<InputsNode>(context, buildEvent, false);
                    IEnumerable<string> buildEventInputsStrings = buildEventInputs.Entries.Select((entry) => GetPath(entry.FilePath));
                    VisualStudioUtils.WriteArrayElement(writer, buildEventInputsStrings, "Inputs");

                    writer.WriteEndElement();
                }

                // TODO: FX compiler support?

                if (hasAnyResourceFiles)
                {
                    writer.WriteStartElement("ResourceCompile");
                    VisualStudioUtils.WritePreprocessorDefinitions(writer, defineEntryStrings, true, VisualStudioUtils.GetConfigCondition(configuration, platform, archName));
                    VisualStudioUtils.WriteAdditionalIncludeDirs(writer, includeDirsEntryStrings);
                    // TODO: Culture support?
                    writer.WriteEndElement();
                }

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

                if (module.Kind == EModuleKind.LibStatic)
                {
                    writer.WriteStartElement("Lib");
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

                if (module.Kind == EModuleKind.LibStatic)
                {
                    writer.WriteElementBoolIfTrue("TreatLibWarningAsErrors", warnings.AreAllWarningsFatal);
                }
                else
                {
                    writer.WriteElementBoolIfTrue("TreatLinkerWarningAsErrors", warnings.AreAllWarningsFatal);
                }

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

                // TODO: support for individual fatal link errors by adding `/wx:a,b,c` automatically?
                IEnumerable<string> linkerOptions = linkOptions.Entries.Select((entry) => entry.Option);
                VisualStudioUtils.WriteArrayElement(writer, linkerOptions, "AdditionalOptions", "%(AdditionalOptions)", " ");

                if (module.Kind == EModuleKind.LibStatic)
                {
                    writer.WriteEndElement();
                }

                if (module.Kind != EModuleKind.LibStatic)
                {
                    if (module.Kind == EModuleKind.LibShared)
                    {
                        string targetDir = GetPath(buildOutput.LibDir);
                        string importLib = Path.Join(targetDir, module.Name + ".lib");
                        writer.WriteElementString("ImportLibrary", importLib);
                    }

                    writer.TryWriteElementString("EntryPointSymbol", module.EntryPoint);
                    writer.WriteElementBoolIfTrue("GenerateMapFile", buildOutput.MakeMapFile);

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

                    writer.WriteStartElement("Manifest");
                    writer.WriteElementString("EnableDpiAwareness", buildOptions.DpiAwarenessMode switch
                    {
                        EDpiAwareMode.None => "false",
                        EDpiAwareMode.HighDpiAware => "true",
                        EDpiAwareMode.PerMonitorHighDpiAware => "PerMonitorHighDPIAware",
                        _ => throw new NotImplementedException($"Unsupported DPI Awareness Mode: {buildOptions.DpiAwarenessMode}"),
                    });
                    IEnumerable<string> extraManifestFiles = files.Entries.Where((entry) => entry.ResolvedFileAction == EFileAction.Manifest).Select((entry) => GetPath(entry.ResolvedFilePath));
                    VisualStudioUtils.WriteArrayElement(writer, extraManifestFiles, "AdditionalManifestFiles", "%(AdditionalManifestFiles)");
                    writer.WriteEndElement();
                }
            }

            void writeBuildEvent(EBuildEvent evt)
            {
                BuildEventNode buildEvent = _projectService.GetMergedNode<BuildEventNode>(context, module, (n) => n.EventName == evt, false);
                List<CommandNode> buildCommands = _projectService.GetNodes<CommandNode>(context, buildEvent, false);
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

                    if (!string.IsNullOrEmpty(buildEvent.Message))
                    {
                        writer.WriteElementString("Message", buildEvent.Message);
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
        });
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

    private void WriteProjectReferences(XmlWriter writer, InvocationContext invocationContext, ModuleNode module)
    {
        writer.WriteStartElement("ItemGroup");

        VisualStudioUtils.ForEachConfig(_helper, (configuration, platform, archName) =>
        {
            ProjectContext context = _projectService.CreateProjectContext(invocationContext, module, configuration, platform);
            BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);

            bool isManaged = VisualStudioUtils.IsManaged(module, buildOptions);
            bool isClrMixed = VisualStudioUtils.IsClrMixed(module, buildOptions);
            List<DependenciesNode> dependencies = _projectService.GetNodes<DependenciesNode>(context, module, module.IsBinary);

            foreach (DependenciesEntryNode depEntry in dependencies.SelectMany((entry) => entry.Entries))
            {
                if (depEntry.Kind != EDependencyKind.Default && depEntry.Kind != EDependencyKind.Link)
                {
                    continue;
                }

                ModuleNode depModule = _projectService.TryGetModule(depEntry.DependencyName)
                    ?? throw new InvalidOperationException($"No module found with name '{depEntry.DependencyName}', but module '{module.Name}' depends on it.");

                writer.WriteStartElement("ProjectReference");
                writer.WriteAttributeString("Include", $"{module.Name}{ProjectExtension}");

                writer.WriteElementString("Project", ModuleGroupTree.GetModuleGuid(depModule));

                if (isManaged || (isClrMixed && depModule.Kind != EModuleKind.LibStatic))
                {
                    writer.WriteElementString("Private", "true");
                    writer.WriteElementString("ReferenceOutputAssembly", "true");
                    writer.WriteElementString("CopyLocalSatelliteAssemblies", "false");
                    writer.WriteElementString("LinkLibraryDependencies", "true");
                    writer.WriteElementString("UseLibraryDependencyInputs", "false");
                }

                writer.WriteEndElement();
            }
        });

        writer.WriteEndElement();
    }

    private void AddGeneratedFilesForCustomBuildRule(ProjectContext context, BuildRuleNode buildRule)
    {
        List<CommandNode> commands = _projectService.GetNodes<CommandNode>(context, buildRule, false);
        if (commands.Count == 0)
        {
            return;
        }

        OutputsNode outputs = _projectService.GetMergedNode<OutputsNode>(context, buildRule, false);
        IEnumerable<string> outputFilePaths = outputs.Entries.Select((entry) => entry.FilePath);

        foreach (string outputFilePath in outputFilePaths)
        {
            EFileAction fileAction = FilesEntryNode.GetDefaultFileAction(outputFilePath);
            EFileBuildRule fileBuildRule = FilesEntryNode.GetDefaultFileBuildRule(outputFilePath);

            foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
            {
                if (fileGroup.CanHandleFile(outputFilePath, fileAction, fileBuildRule))
                {
                    fileGroup.AddGeneratedFile(context, outputFilePath, string.Empty, fileAction, fileBuildRule);
                    break;
                }
            }
        }
    }

    private void CreateFileGroups(InvocationContext invocationContext, ModuleNode module)
    {
        _fileGroups = VisualStudioUtils.CreateFileGroups(_helper, _outputPath);

        VisualStudioUtils.ForEachConfig(_helper, (configuration, platform, archName) =>
        {
            ProjectContext context = _projectService.CreateProjectContext(invocationContext, module, configuration, platform);
            FilesNode files = _projectService.GetMergedNode<FilesNode>(context, module);

            foreach (FilesEntryNode entry in files.Entries)
            {
                // Search for a file group that can handle this file entry
                bool foundGroup = false;
                foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
                {
                    if (fileGroup.CanHandleFile(entry))
                    {
                        fileGroup.AddFile(context, entry);
                        foundGroup = true;
                        break;
                    }
                }

                // This should never happen because the NoneFileGroup can handle any file
                if (!foundGroup)
                {
                    throw new Exception("No file group found for file: " + entry.ResolvedFilePath);
                }

                // Add each output file from custom build rules
                if (!entry.IsExcludedFromBuild && entry.ResolvedFileBuildRule == EFileBuildRule.Custom)
                {
                    BuildRuleNode buildRule = _projectService.GetMergedNode<BuildRuleNode>(context, module, (n) => n.RuleName == entry.BuildRuleName, false);
                    if (buildRule.RuleName != entry.BuildRuleName)
                    {
                        throw new Exception($"No build rule with the name '{entry.BuildRuleName}' was found.");
                    }

                    if (buildRule.LinkOutput)
                    {
                        AddGeneratedFilesForCustomBuildRule(context, buildRule);
                    }
                }
            }
        });

        foreach (IVisualStudioFileGroup fileGroup in _fileGroups)
        {
            fileGroup.SortFiles();
            fileGroup.SetupVirtualPaths();
        }
    }
}
