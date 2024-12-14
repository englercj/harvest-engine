// Copyright Chad Engler

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
        //m.itemDefinitionGroups,
        //m.assemblyReferences,
        //m.files,
        //m.projectReferences,
        //m.importLanguageTargets,
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
            string configName = $"{configuration.ConfigName} {platform.PlatformName}";
            writer.WriteStartElement("ProjectConfiguration");
            writer.WriteAttributeString("Include", $"{configName}|{archName}");
            writer.WriteElementString("Configuration", configName);
            writer.WriteElementString("Platform", archName);
            writer.WriteEndElement();
        });

        writer.WriteEndElement();
    }

    private void WriteGlobals(XmlWriter writer, InvocationContext invocationContext, ModuleNode module, Guid id)
    {
        bool isWindows = _helper.Platforms.Where((n) => n.System == EPlatformSystem.Windows).Any();
        ProjectContext context = _projectService.GetProjectContext(invocationContext, module);

        SystemNode windowsSystemNode = _projectService.GetResolvedNode<SystemNode>(context, module, (n) => n.System == EPlatformSystem.Windows);
        SystemNode dotnetSystemNode = _projectService.GetResolvedNode<SystemNode>(context, module, (n) => n.System == EPlatformSystem.DotNet);
        ToolsetNode toolsetNode = _projectService.GetResolvedNode<ToolsetNode>(context, module, (n) => n.Toolset == EToolset.MSVC);

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
            ToolsetNode toolsetNode = _projectService.GetResolvedNode<ToolsetNode>(context, module, (n) => n.Toolset == platform.Toolset);

            string configName = $"{configuration.ConfigName} {platform.PlatformName}";
            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", $"'$(Configuration)|$(Platform)'=='{configName}|{archName}'");
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
                RuntimeNode runtimeNode = _projectService.GetResolvedNode<RuntimeNode>(context, module);
                if (runtimeNode.Runtime == ERuntime.Debug || (runtimeNode.Runtime == ERuntime.Default && configuration.ConfigName == "Debug"))
                {
                    writer.WriteElementString("UseDebugLibraries", "true");
                }
                else
                {
                    writer.WriteElementString("UseDebugLibraries", "false");
                }

                if (module.MfcMode == EModuleMfcMode.Static)
                {
                    writer.WriteElementString("UseOfMfc", "Static");
                }
                else if (module.MfcMode == EModuleMfcMode.Static)
                {
                    writer.WriteElementString("UseOfMfc", "Dynamic");
                }

                if (module.AtlMode == EModuleAtlMode.Static)
                {
                    writer.WriteElementString("UseOfAtl", "Static");
                }
                else if (module.AtlMode == EModuleAtlMode.Dynamic)
                {
                    writer.WriteElementString("UseOfAtl", "Dynamic");
                }

                switch (module.ClrMode)
                {
                    case EModuleClrMode.On:
                        writer.WriteElementString("CLRSupport", "true");
                        break;
                    case EModuleClrMode.Off:
                        // Always enabled for csharp code
                        if (module.Language == EModuleLanguage.CSharp)
                        {
                            writer.WriteElementString("CLRSupport", "true");
                        }
                        break;
                    case EModuleClrMode.NetCore:
                        writer.WriteElementString("CLRSupport", "NetCore");
                        break;
                }

                SanitizeNode sanitize = _projectService.GetResolvedNode<SanitizeNode>(context, module);
                if (sanitize.EnableAddress)
                {
                    writer.WriteElementString("EnableASAN", "true");
                }
                if (sanitize.EnableFuzzer)
                {
                    writer.WriteElementString("EnableFuzzer", "true");
                }

                OptimizeNode optimize = _projectService.GetResolvedNode<OptimizeNode>(context, module);
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
                BuildOutputNode buildOutput = _projectService.GetResolvedNode<BuildOutputNode>(context, module);
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
            string configName = $"{configuration.ConfigName} {platform.PlatformName}";
            writer.WriteStartElement("ImportGroup");
            writer.WriteAttributeString("Label", "PropertySheets");
            writer.WriteAttributeString("Condition", $"'$(Configuration)|$(Platform)'=='{configName}|{archName}'");

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
            BuildOutputNode buildOutput = _projectService.GetResolvedNode<BuildOutputNode>(context, module);

            string configName = $"{configuration.ConfigName} {platform.PlatformName}";
            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", $"'$(Configuration)|$(Platform)'=='{configName}|{archName}'");

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

            // TODO: Should this be a module property? Project property?
            //if (module.ExtensionsToClean.Count > 0)
            //{
            //    StringBuilder cleanStr = new();
            //    foreach (string ext in module.ExtensionsToClean)
            //    {
            //        cleanStr.Append('*');
            //        cleanStr.Append(ext);
            //        cleanStr.Append(';');
            //    }
            //    writer.WriteElementString("ExtensionsToDeleteOnClient", $"$(ExtensionsToDeleteOnClean)");
            //}

            if (module.Kind != EModuleKind.Custom)
            {
            }

            writer.WriteEndElement();
        });
    }
}
