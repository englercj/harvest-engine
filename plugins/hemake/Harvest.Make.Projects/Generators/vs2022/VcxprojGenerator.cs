// Copyright Chad Engler

using Harvest.Make.Extensions;
using Harvest.Make.Projects.Nodes;
using System.CommandLine.Invocation;
using System.Text;
using System.Xml;

namespace Harvest.Make.Projects.Generators.vs2022;

internal class VcxprojGenerator(IProjectService projectService, ProjectGeneratorHelper helper)
{
    private readonly IProjectService _projectService = projectService;
    private readonly ProjectGeneratorHelper _helper = helper;
    private string _outputPath = "";

    public void Generate(InvocationContext context, ModuleNode module, Guid id)
    {
        _outputPath = Path.Join(_helper.BuildOutput.ProjectDir, $"{module.ModuleName}.vcxproj");

        using FileStream stream = new(_outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
        using XmlWriter writer = XmlWriter.Create(stream, new XmlWriterSettings { Encoding = Encoding.UTF8, Indent = true });

        writer.WriteStartDocument();
        writer.WriteStartElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");
        writer.WriteAttributeString("DefaultTargets", "Build");

        WriteConfigurations(writer);
        WriteGlobals(writer, context, module, id);
        WriteImportDefaultProperties(writer);
        WriteConfigurationProperties(writer, context, module);
        WriteImportProperties(writer);
        WriteUserMacros(writer);
        WriteOutputProperties(writer, context, module);
        WriteItemDefinitionGroups(writer, context, module);
        //m.assemblyReferences,
        //m.files,
        //m.projectReferences,
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
        //m.importExtensionTargets,
        //m.ensureNuGetPackageBuildImports,

        writer.WriteEndElement(); // Project
    }

    private void WriteConfigurations(XmlWriter writer)
    {
        writer.WriteStartElement("ItemGroup");
        writer.WriteAttributeString("Label", "ProjectConfigurations");

        VisualStudioUtils.ForEachConfig(_helper, (ConfigurationNode configuration, PlatformNode platform, string archName) =>
        {
            writer.WriteStartElement("ProjectConfiguration");
            writer.WriteAttributeString("Include", VisualStudioUtils.GetConfigName(configuration, platform, archName));
            writer.WriteElementString("Configuration", VisualStudioUtils.GetConfigName(configuration, platform));
            writer.WriteElementString("Platform", archName);
            writer.WriteEndElement();
        });

        writer.WriteEndElement();
    }

    private void WriteGlobals(XmlWriter writer, InvocationContext invocationContext, ModuleNode module, Guid id)
    {
        bool isWindows = _helper.Platforms.Where((n) => n.System == EPlatformSystem.Windows).Any();
        ProjectContext context = _projectService.GetProjectContext(invocationContext, module);

        SystemNode windowsSystemNode = _projectService.GetMergedNode<SystemNode>(context, module, (n) => n.System == EPlatformSystem.Windows);
        SystemNode dotnetSystemNode = _projectService.GetMergedNode<SystemNode>(context, module, (n) => n.System == EPlatformSystem.DotNet);
        ToolsetNode toolsetNode = _projectService.GetMergedNode<ToolsetNode>(context, module, (n) => n.Toolset == EToolset.MSVC);

        writer.WriteStartElement("PropertyGroup");
        writer.WriteAttributeString("Label", "Globals");

        // TODO: Is this needed?
        //writer.WriteElementString("IgnoreWarnCompileDuplicatedFilename", "true");

        if (isWindows)
        {
            bool isManaged = VisualStudioUtils.IsManaged(module);

            if (isManaged || VisualStudioUtils.IsClrMixed(module))
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

        writer.WriteElementString("ProjectGuid", $"{{{id}}}");
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

        writer.WriteEndElement(); // PropertyGroup
    }

    private void WriteImportDefaultProperties(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
        writer.WriteEndElement();
    }

    private void WriteConfigurationProperties(XmlWriter writer, InvocationContext invocationContext, ModuleNode module)
    {
        VisualStudioUtils.ForEachConfig(_helper, (ConfigurationNode configuration, PlatformNode platform, string archName) =>
        {
            ProjectContext context = _projectService.GetProjectContext(invocationContext, module, configuration, platform);
            BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);
            ToolsetNode toolsetNode = _projectService.GetMergedNode<ToolsetNode>(context, module, (n) => n.Toolset == platform.Toolset);

            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", VisualStudioUtils.GetConfigCondition(configuration, platform, archName));
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
                // TODO: Translate path to use VS tokens.
                BuildOutputNode buildOutput = _projectService.GetMergedNode<BuildOutputNode>(context, module);
                string outDir = Path.GetRelativePath(_outputPath, module.Kind switch
                {
                    EModuleKind.AppConsole => buildOutput.BinDir,
                    EModuleKind.AppWindowed => buildOutput.BinDir,
                    EModuleKind.Content => buildOutput.BinDir,
                    EModuleKind.Custom => buildOutput.BinDir,
                    EModuleKind.LibHeader => buildOutput.LibDir,
                    EModuleKind.LibStatic => buildOutput.LibDir,
                    EModuleKind.LibShared => buildOutput.LibDir,
                    _ => buildOutput.LibDir,
                });
                writer.WriteElementString("OutDir", $"{outDir}\\");

                // TODO: Translate path to use VS tokens.
                string objDir = Path.GetRelativePath(_outputPath, buildOutput.ObjDir);
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

        writer.WriteStartElement("ImportGroup");
        writer.WriteAttributeString("Label", "ExtensionSettings");
        writer.WriteEndElement();

        VisualStudioUtils.ForEachConfig(_helper, (ConfigurationNode configuration, PlatformNode platform, string archName) =>
        {
            writer.WriteStartElement("ImportGroup");
            writer.WriteAttributeString("Label", "PropertySheets");
            writer.WriteAttributeString("Condition", VisualStudioUtils.GetConfigCondition(configuration, platform, archName));

            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
            writer.WriteAttributeString("Condition", $"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
            writer.WriteAttributeString("Label", "LocalAppDataPlatform");
            writer.WriteEndElement();

            writer.WriteEndElement();
        });
    }

    private void WriteUserMacros(XmlWriter writer)
    {
        writer.WriteStartElement("PropertyGroup");
        writer.WriteAttributeString("Label", "UserMacros");
        writer.WriteEndElement();
    }

    private void WriteOutputProperties(XmlWriter writer, InvocationContext invocationContext, ModuleNode module)
    {
        VisualStudioUtils.ForEachConfig(_helper, (ConfigurationNode configuration, PlatformNode platform, string archName) =>
        {
            ProjectContext context = _projectService.GetProjectContext(invocationContext, module, configuration, platform);
            BuildOutputNode buildOutput = _projectService.GetMergedNode<BuildOutputNode>(context, module);
            BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);
            LinkOptionsNode linkOptions = _projectService.GetMergedNode<LinkOptionsNode>(context, module);
            OptimizeNode optimize = _projectService.GetMergedNode<OptimizeNode>(context, module);

            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", VisualStudioUtils.GetConfigCondition(configuration, platform, archName));

            // TODO: Translate path to use VS tokens.
            string outDir = Path.GetRelativePath(_outputPath, module.Kind switch
            {
                EModuleKind.AppConsole => buildOutput.BinDir,
                EModuleKind.AppWindowed => buildOutput.BinDir,
                EModuleKind.Content => buildOutput.BinDir,
                EModuleKind.Custom => buildOutput.BinDir,
                EModuleKind.LibHeader => buildOutput.LibDir,
                EModuleKind.LibStatic => buildOutput.LibDir,
                EModuleKind.LibShared => buildOutput.LibDir,
                _ => buildOutput.LibDir,
            });
            writer.WriteElementString("OutDir", $"{outDir}\\");

            // TODO: Translate path to use VS tokens.
            string objDir = Path.GetRelativePath(_outputPath, buildOutput.ObjDir);
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

                IncludeDirsNode externalIncludeDirs = _projectService.GetMergedNode<IncludeDirsNode>(context, module, (node) => node.IsExternal);
                IEnumerable<IncludeDirsEntryNode> externalIncludeEntries = externalIncludeDirs.Entries.Where((entry) => entry.IsExternal != EIncludeDirIsExternal.False);
                IEnumerable<string> externalIncludePaths = externalIncludeEntries.Select((entry) => entry.Path);
                string externalIncludePathsString = string.Join(';', externalIncludePaths);
                if (!string.IsNullOrEmpty(externalIncludePathsString))
                {
                    writer.WriteElementString("ExternalIncludePath", $"{externalIncludePathsString};$(ExternalIncludePath)");
                }

                LibDirsNode systemLibDirs = _projectService.GetMergedNode<LibDirsNode>(context, module, (node) => node.IsSystem);
                IEnumerable<LibDirsEntryNode> systemLibEntries = systemLibDirs.Entries.Where((entry) => entry.IsSystem != ELibDirIsSystem.False);
                IEnumerable<string> systemLibPaths = systemLibEntries.Select((entry) => entry.Path);
                string systemLibPathsString = string.Join(';', systemLibPaths);
                if (!string.IsNullOrEmpty(systemLibPathsString))
                {
                    writer.WriteElementString("LibraryPath", $"{systemLibPathsString};$(LibraryPath)");
                }

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
        StringBuilder stringBuilder = new();

        VisualStudioUtils.ForEachConfig(_helper, (ConfigurationNode configuration, PlatformNode platform, string archName) =>
        {
            ProjectContext context = _projectService.GetProjectContext(invocationContext, module, configuration, platform);

            writer.WriteStartElement("ItemDefinitionGroup");
            writer.WriteAttributeString("Condition", VisualStudioUtils.GetConfigCondition(configuration, platform, archName));

            if (module.Kind != EModuleKind.Custom)
            {
                BuildOptionsNode buildOptions = _projectService.GetMergedNode<BuildOptionsNode>(context, module);
                CodegenNode codegen = _projectService.GetMergedNode<CodegenNode>(context, module);
                DefinesNode defines = _projectService.GetMergedNode<DefinesNode>(context, module);
                DialectNode dialect = _projectService.GetMergedNode<DialectNode>(context, module);
                ExceptionsNode exceptions = _projectService.GetMergedNode<ExceptionsNode>(context, module);
                ExternalNode external = _projectService.GetMergedNode<ExternalNode>(context, module);
                FloatingPointNode floatingPoint = _projectService.GetMergedNode<FloatingPointNode>(context, module);
                IncludeDirsNode includeDirs = _projectService.GetMergedNode<IncludeDirsNode>(context, module);
                OptimizeNode optimize = _projectService.GetMergedNode<OptimizeNode>(context, module);
                RuntimeNode runtime = _projectService.GetMergedNode<RuntimeNode>(context, module);
                SymbolsNode symbols = _projectService.GetMergedNode<SymbolsNode>(context, module);
                ToolsetNode toolset = _projectService.GetMergedNode<ToolsetNode>(context, module, (n) => n.Toolset == platform.Toolset);
                WarningsNode warnings = _projectService.GetMergedNode<WarningsNode>(context, module);

                bool isOptimizedBuild = VisualStudioUtils.IsOptimizedBuild(optimize);
                bool isDebugBuild = VisualStudioUtils.IsDebugBuild(optimize, symbols);

                writer.WriteStartElement("ClCompile");

                if (buildOptions.PchInclude is null)
                {
                    writer.WriteElementString("PrecompiledHeader", "NotUsing");
                }
                else
                {
                    writer.WriteElementString("PrecompiledHeader", "Use");
                    writer.WriteElementString("PrecompiledHeaderFile", buildOptions.PchInclude);

                    // TODO: When writing out the configuration for individual files, need to output this for the pchsource:
                    //writer.WriteStartElement("PrecompiledHeader");
                    //writer.WriteAttributeString("Condition", VisualStudioUtils.GetConfigCondition(configuration, platform, archName));
                    //writer.WriteString("Create");
                    //writer.WriteEndElement();
                }

                writer.WriteElementString("WarningLevel", VisualStudioUtils.GetWarningLevelString(warnings.WarningsLevel));
                if (warnings.AreAllWarningsFatal)
                {
                    writer.WriteElementString("TreatWarningAsError", "true");
                }

                IEnumerable<string> disabledWarnings = warnings.Entries.Where((entry) => !entry.IsEnabled).Select((entry) => entry.WarningName);
                stringBuilder.Clear();
                stringBuilder.AppendJoin(';', disabledWarnings);
                if (stringBuilder.Length != 0)
                {
                    writer.WriteElementString("DisableSpecificWarnings", $"{stringBuilder};%(DisableSpecificWarnings)");
                }

                IEnumerable<string> fatalWarnings = warnings.Entries.Where((entry) => entry.IsEnabled && entry.IsFatal).Select((entry) => entry.WarningName);
                stringBuilder.Clear();
                stringBuilder.AppendJoin(';', fatalWarnings);
                if (stringBuilder.Length != 0)
                {
                    writer.WriteElementString("TreatSpecificWarningsAsErrors", $"{stringBuilder};%(TreatSpecificWarningsAsErrors)");
                }

                // TODO: When we get to additional compile options, we need to output this:
                //IEnumerable<string> enabledWarnings = warnings.Entries.Where((entry) => !entry.IsEnabled).Select((entry) => $"/w{entry.WarningName}");
                //stringBuilder.Clear();
                //stringBuilder.AppendJoin(' ', enabledWarnings);
                //if (stringBuilder.Length != 0)
                //{
                //    writer.WriteElementString("ExtraCompilerOptionsOrWhatever", stringBuilder.ToString());
                //}

                if (isOptimizedBuild && runtime.Runtime == ERuntime.Debug)
                {
                    writer.WriteElementString("BasicRuntimeChecks", "Default");
                }

                IEnumerable<string> defineEntryStrings = defines.Entries.Select((entry) => entry.Define.Replace("\"", "\\\""));
                stringBuilder.Clear();
                stringBuilder.AppendJoin(';', defineEntryStrings);
                if (exceptions.ExceptionsMode == EExceptionsMode.Off)
                {
                    stringBuilder.Append(";_HAS_EXCEPTIONS=0");
                }
                if (stringBuilder.Length != 0)
                {
                    writer.WriteElementString("PreprocessorDefinitions", $"{stringBuilder};%(PreprocessorDefinitions)");
                }

                // TODO: Support for undefines?
                //IEnumerable<string> undefineEntryStrings = undefines.Entries.Select((entry) => entry.Define.Replace("\"", "\\\""));
                //stringBuilder.Clear();
                //stringBuilder.AppendJoin(';', undefineEntryStrings);
                //if (stringBuilder.Length != 0)
                //{
                //    writer.WriteElementString("UndefinePreprocessorDefinitions", $"{stringBuilder};%(UndefinePreprocessorDefinitions)");
                //}

                // TODO: Translate paths to use VS tokens.
                IEnumerable<string> includeDirsEntryStrings = includeDirs.Entries.Select((entry) => entry.Path);
                stringBuilder.Clear();
                stringBuilder.AppendJoin(';', includeDirsEntryStrings);
                if (stringBuilder.Length != 0)
                {
                    writer.WriteElementString("AdditionalIncludeDirectories", $"{stringBuilder};%(AdditionalIncludeDirectories)");
                }

                // TODO: Support for forced includes?
                // TODO: Translate paths to use VS tokens.
                //IEnumerable<string> forceIncludesEntryStrings = includeDirs.Entries.Select((entry) => entry.Path);
                //stringBuilder.Clear();
                //stringBuilder.AppendJoin(';', forceIncludesEntryStrings);
                //if (stringBuilder.Length != 0)
                //{
                //    writer.WriteElementString("ForcedIncludeFiles", stringBuilder.ToString());
                //}

                // TODO: Support for using directories (C++/CLI, #using "")?
                // TODO: Translate paths to use VS tokens.
                //IEnumerable<string> usingDirsEntryStrings = usingDirs.Entries.Select((entry) => entry.Path);
                //stringBuilder.Clear();
                //stringBuilder.AppendJoin(';', usingDirsEntryStrings);
                //if (stringBuilder.Length != 0)
                //{
                //    writer.WriteElementString("AdditionalUsingDirectories", $"{stringBuilder};%(AdditionalUsingDirectories)");
                //}

                // TODO: Support for forced usings (C++/CLI, #using "")?
                // TODO: Translate paths to use VS tokens.
                //IEnumerable<string> forceUsingsEntryStrings = includeDirs.Entries.Select((entry) => entry.Path);
                //stringBuilder.Clear();
                //stringBuilder.AppendJoin(';', forceUsingsEntryStrings);
                //if (stringBuilder.Length != 0)
                //{
                //    writer.WriteElementString("ForcedUsingFiles", stringBuilder.ToString());
                //}

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

                writer.WriteElementBool("RuntimeTypeInfo", buildOptions.RuntimeTypeInfo);

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
                stringBuilder.Clear();
                stringBuilder.AppendJoin(' ', buildOptionEntryStrings);
                if (platform.Toolset == EToolset.Clang)
                {
                    // <OpenMPSupport> is ignored when using the clang toolset
                    stringBuilder.Append(" /openmp");
                }

                if (stringBuilder.Length > 0)
                {
                    writer.WriteElementString("AdditionalOptions", $"{stringBuilder} %(AdditionalOptions)");
                }

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
                //if (buildOptions.UseFullPaths is not null)
                //{
                //    writer.WriteElementBool("UseFullPaths", buildOptions.UseFullPaths.Value);
                //}

                // TODO: Support for RemoveUnreferencedCodeData?
                //if (buildOptions.RemoveUnreferencedCodeData is not null)
                //{
                //    writer.WriteElementBool("RemoveUnreferencedCodeData", buildOptions.RemoveUnreferencedCodeData.Value);
                //}

                // TODO: Support for CompileAsWinRT?
                //if (buildOptions.CompileAsWinRT is not null)
                //{
                //    writer.WriteElementBool("CompileAsWinRT", buildOptions.CompileAsWinRT.Value);
                //}

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

                    IEnumerable<string> buildCommandStrings = buildCommands.Select((entry) => entry.GetCommandString()).OfType<string>();
                    stringBuilder.Clear();
                    stringBuilder.AppendJoin("\r\n", buildCommandStrings);
                    writer.WriteElementString("Command", stringBuilder.ToString());

                    OutputsNode buildEventOutputs = _projectService.GetMergedNode<OutputsNode>(context, buildEvent, false);
                    IEnumerable<string> buildEventOutputsStrings = buildEventOutputs.Entries.Select((entry) => entry.FilePath);
                    stringBuilder.Clear();
                    stringBuilder.AppendJoin(';', buildEventOutputsStrings);
                    if (stringBuilder.Length != 0)
                    {
                        writer.WriteElementString("Outputs", stringBuilder.ToString());
                    }

                    // TODO: Make paths relative to project file.
                    InputsNode buildEventInputs = _projectService.GetMergedNode<InputsNode>(context, buildEvent, false);
                    IEnumerable<string> buildEventInputsStrings = buildEventInputs.Entries.Select((entry) => entry.FilePath);
                    stringBuilder.Clear();
                    stringBuilder.AppendJoin(';', buildEventInputsStrings);
                    if (stringBuilder.Length != 0)
                    {
                        writer.WriteElementString("Inputs", stringBuilder.ToString());
                    }

                    writer.WriteEndElement();
                }

                //m.fxCompile,
                //m.resourceCompile,
                //m.linker,
                //m.manifest,
            }

            //m.ruleVars,
            //m.buildEvents,
            //m.buildLog,

            writer.WriteEndElement();
        });
    }

}
