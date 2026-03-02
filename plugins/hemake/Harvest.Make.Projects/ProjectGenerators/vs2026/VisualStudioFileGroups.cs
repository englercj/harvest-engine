// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using System.Xml;
using static Harvest.Make.Projects.ProjectGenerators.vs2026.IVisualStudioFileGroup;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

internal class ClIncludeFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 10;
    public override string GroupTag => "ClInclude";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Include;
    }
}

internal class ClCompileFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 20;
    public override string GroupTag => "ClCompile";

    private Dictionary<string, int> _objectFileNameSequence = [];

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Build
            && buildRule switch
            {
                EFileBuildRule.C => true,
                EFileBuildRule.Cpp => true,
                EFileBuildRule.CSharp => true,
                EFileBuildRule.ObjC => true,
                EFileBuildRule.ObjCpp => true,
                _ => false
            };
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        if (file.IsExcludedFromBuild)
        {
            HandleExcludedFile(writer, file, projectTree, archName);
            return;
        }

        string condition = VisualStudioUtils.GetConfigCondition(projectTree, archName);
        string fileBaseName = Path.GetFileNameWithoutExtension(file.FullPath);

        // Need to make the obj file name unique in the case of two files with the same base name.
        // This is because a project with 'src/a.cpp' and 'test/a.cpp' will both output to 'a.obj'.
        if (_objectFileNameSequence.TryGetValue(fileBaseName, out int sequence))
        {
            _objectFileNameSequence[fileBaseName] = sequence + 1;
            VisualStudioUtils.WriteElementString(writer, "ObjectFileName", $"$(IntDir)\\{fileBaseName}{sequence}.obj", condition);
        }
        else
        {
            _objectFileNameSequence.Add(fileBaseName, 1);
        }

        BuildOptionsNode buildOptions = file.ProjectTree.GetMergedNode<BuildOptionsNode>(file.Module.Node);
        if (buildOptions.PchSource == file.FullPath)
        {
            VisualStudioUtils.WriteElementString(writer, "PrecompiledHeader", "Create", condition);
        }

        if (buildOptions.ClrMode != EBuildClrMode.Off)
        {
            VisualStudioUtils.WriteElementString(writer, "CompileAsManaged", "true", condition);
        }

        // TODO: File-level configuration? How do I only search in the file context, and not the parent scope?
        // Maybe need a new project service function that doesn't search aprent scopes; just tries to find a node in
        // the given scope and it if it fails returns null. That way I can tell if the file has a config or not.

        // TODO: Most of these duplicate code from VcxprojGenerator.cs, probably should make utilities in VisualStudioUtils.
        // TODO: Also need to ensure that all the elements output here actually use the `condition`.

        //CodegenNode codegen = file.ProjectTree.GetMergedNode<CodegenNode>(file.Module.Node);
        //DefinesNode defines = file.ProjectTree.GetMergedNode<DefinesNode>(file.Module.Node);
        //DialectNode dialect = file.ProjectTree.GetMergedNode<DialectNode>(file.Module.Node);
        //ExceptionsNode exceptions = file.ProjectTree.GetMergedNode<ExceptionsNode>(file.Module.Node);
        //OptimizeNode optimize = file.ProjectTree.GetMergedNode<OptimizeNode>(file.Module.Node);
        //RuntimeNode runtime = file.ProjectTree.GetMergedNode<RuntimeNode>(file.Module.Node);
        //WarningsNode warnings = file.ProjectTree.GetMergedNode<WarningsNode>(file.Module.Node);

        //IEnumerable<string> defineEntryStrings = defines.Entries.Select((entry) => entry.Define);
        //if (exceptions.ExceptionsMode == EExceptionsMode.Off)
        //{
        //    defineEntryStrings = defineEntryStrings.Concat(["_HAS_EXCEPTIONS=0"]);
        //}
        //VisualStudioUtils.WritePreprocessorDefinitions(writer, defineEntryStrings, false, condition);

        //IEnumerable<string> undefineEntryStrings = undefines.Entries.Select((entry) => entry.Define);
        //WriteArrayElement(writer, undefineEntryStrings, "UndefinePreprocessorDefinitions", "%(UndefinePreprocessorDefinitions)");

        //if (dialect.CDialect != ECDialect.Default)
        //{
        //    VisualStudioUtils.WriteArrayElement(writer, "LanguageStandard_C", dialect.CDialect switch
        //    {
        //        ECDialect.C11 => "stdc11",
        //        ECDialect.C17 => "stdc17",
        //        ECDialect.C23 => "stdclatest",
        //        _ => throw new NotImplementedException($"Unsupported C Dialect: {dialect.CDialect}"),
        //    }, condition);
        //}

        //if (dialect.CppDialect != ECppDialect.Default)
        //{
        //    VisualStudioUtils.WriteArrayElement(writer, "LanguageStandard", dialect.CppDialect switch
        //    {
        //        ECppDialect.Cpp14 => "stdcpp14",
        //        ECppDialect.Cpp17 => "stdcpp17",
        //        ECppDialect.Cpp20 => "stdcpp20",
        //        ECppDialect.Cpp23 => "stdcpplatest",
        //        _ => throw new NotImplementedException($"Unsupported C++ Dialect: {dialect.CppDialect}"),
        //    }, condition);
        //}

        //VisualStudioUtils.WriteArrayElement(writer, "Optimization", optimize.OptimizationLevel switch
        //{
        //    EOptimizationLevel.Default => "Disabled",
        //    EOptimizationLevel.Debug => "Disabled",
        //    EOptimizationLevel.Off => "Disabled",
        //    EOptimizationLevel.On => "Full",
        //    EOptimizationLevel.Size => "MinSpace",
        //    EOptimizationLevel.Speed => "MaxSpeed",
        //    _ => throw new NotImplementedException($"Unsupported Optimization Level: {optimize.OptimizationLevel}"),
        //}, condition);

        // TODO: Support for forced includes?
        //IEnumerable<string> forceIncludesEntryStrings = includeDirs.Entries.Select((entry) => GetPath(entry.Path));
        //WriteArrayElement(writer, forceIncludesEntryStrings, "ForcedIncludeFiles");

        // TODO: Support for using directories (C++/CLI, #using "")?
        //IEnumerable<string> usingDirsEntryStrings = usingDirs.Entries.Select((entry) => GetPath(entry.Path));
        //WriteArrayElement(writer, usingDirsEntryStrings, "AdditionalUsingDirectories", "%(AdditionalUsingDirectories)");

        //if (platform.Arch == EPlatformArch.X86)
        //{
        //    VisualStudioUtils.WriteArrayElement(writer, "EnableEnhancedInstructionSet", codegen.CodegenMode switch
        //    {
        //        ECodegenMode.AVX => "AdvancedVectorExtensions",
        //        ECodegenMode.AVX2 => "AdvancedVectorExtensions2",
        //        ECodegenMode.AVX512 => "AdvancedVectorExtensions512",
        //        ECodegenMode.SSE => "StreamingSIMDExtensions",
        //        ECodegenMode.SSE2 => "StreamingSIMDExtensions2",
        //        ECodegenMode.SSE3 => "StreamingSIMDExtensions2",
        //        ECodegenMode.SSSE3 => "StreamingSIMDExtensions2",
        //        ECodegenMode.SSE41 => "StreamingSIMDExtensions2",
        //        ECodegenMode.SSE42 => "StreamingSIMDExtensions2",
        //        _ => throw new NotImplementedException($"Unsupported instruction set for x86: {codegen.CodegenMode}"),
        //    }, condition);
        //}
        //else if (platform.Arch == EPlatformArch.X86_64)
        //{
        //    VisualStudioUtils.WriteArrayElement(writer, "EnableEnhancedInstructionSet", codegen.CodegenMode switch
        //    {
        //        ECodegenMode.AVX => "AdvancedVectorExtensions",
        //        ECodegenMode.AVX2 => "AdvancedVectorExtensions2",
        //        ECodegenMode.AVX512 => "AdvancedVectorExtensions512",
        //        _ => throw new NotImplementedException($"Unsupported instruction set for x86_64: {codegen.CodegenMode}"),
        //    }, condition);
        //}
        //else if (platform.Arch == EPlatformArch.Arm64)
        //{
        //    VisualStudioUtils.WriteArrayElement(writer, "EnableEnhancedInstructionSet", codegen.CodegenMode switch
        //    {
        //        ECodegenMode.ARMv8_0 => "CPUExtensionRequirementsARMv80",
        //        ECodegenMode.ARMv8_1 => "CPUExtensionRequirementsARMv81",
        //        ECodegenMode.ARMv8_2 => "CPUExtensionRequirementsARMv82",
        //        ECodegenMode.ARMv8_3 => "CPUExtensionRequirementsARMv83",
        //        ECodegenMode.ARMv8_4 => "CPUExtensionRequirementsARMv84",
        //        ECodegenMode.ARMv8_5 => "CPUExtensionRequirementsARMv85",
        //        ECodegenMode.ARMv8_6 => "CPUExtensionRequirementsARMv86",
        //        ECodegenMode.ARMv8_7 => "CPUExtensionRequirementsARMv87",
        //        ECodegenMode.ARMv8_8 => "CPUExtensionRequirementsARMv88",
        //        ECodegenMode.ARMv9_0 => "CPUExtensionRequirementsARMv90",
        //        ECodegenMode.ARMv9_1 => "CPUExtensionRequirementsARMv90",
        //        ECodegenMode.ARMv9_2 => "CPUExtensionRequirementsARMv90",
        //        ECodegenMode.ARMv9_3 => "CPUExtensionRequirementsARMv90",
        //        ECodegenMode.ARMv9_4 => "CPUExtensionRequirementsARMv90",
        //        _ => throw new NotImplementedException($"Unsupported instruction set for ARM64: {codegen.CodegenMode}"),
        //    }, condition);
        //}

        //IEnumerable<string> buildOptionEntryStrings = buildOptions.Entries.Select((entry) => entry.Option);
        //if (platform.Toolset == EToolset.Clang)
        //{
        //    // <OpenMPSupport> is ignored when using the clang toolset so we need to add it here
        //    buildOptionEntryStrings = buildOptionEntryStrings.Concat(["/openmp"]);
        //}
        //IEnumerable<string> enabledWarnings = warnings.Entries.Where((entry) => entry.IsEnabled).Select((entry) => $"/w{entry.WarningName}");
        //buildOptionEntryStrings = buildOptionEntryStrings.Concat(enabledWarnings);
        //VisualStudioUtils.WriteArrayElement(writer, buildOptionEntryStrings, "AdditionalOptions", "%(AdditionalOptions)", " ", condition);

        //IEnumerable<string> disabledWarnings = warnings.Entries.Where((entry) => !entry.IsEnabled).Select((entry) => entry.WarningName);
        //VisualStudioUtils.WriteArrayElement(writer, disabledWarnings, "DisableSpecificWarnings", "%(DisableSpecificWarnings)", ";", condition);

        //IEnumerable<string> fatalWarnings = warnings.Entries.Where((entry) => entry.IsEnabled && entry.IsFatal).Select((entry) => entry.WarningName);
        //VisualStudioUtils.WriteArrayElement(writer, fatalWarnings, "TreatSpecificWarningsAsErrors", "%(TreatSpecificWarningsAsErrors)", ";", condition);

        //bool isOptimizedBuild = VisualStudioUtils.IsOptimizedBuild(optimize);
        //if (isOptimizedBuild && runtime.Runtime == ERuntime.Debug)
        //{
        //    VisualStudioUtils.WriteElementString(writer, "BasicRuntimeChecks", "Default", condition);
        //}

        //if (exceptions.ExceptionsMode != EExceptionsMode.Default)
        //{
        //    VisualStudioUtils.WriteElementString(writer, "ExceptionHandling", exceptions.ExceptionsMode switch
        //    {
        //        EExceptionsMode.On => "Sync",
        //        EExceptionsMode.Off => "false",
        //        EExceptionsMode.SEH => "Async",
        //        _ => throw new NotImplementedException($"Unsupported Exceptions Mode: {exceptions.ExceptionsMode}"),
        //    }, condition);
        //}

        //VisualStudioUtils.WriteElementString(writer, "CompileAs", file.Module?.Language switch
        //{
        //    EModuleLanguage.C => "CompileAsC",
        //    EModuleLanguage.Cpp => "CompileAsCpp",
        //    _ => throw new NotImplementedException($"Unsupported Language: {file.Module?.Language}"),
        //}, condition);

        //if (!buildOptions.RuntimeTypeInfo && buildOptions.ClrMode == EBuildClrMode.Off)
        //{
        //    VisualStudioUtils.WriteElementString(writer, "RuntimeTypeInfo", "false", condition);
        //}
        //else if (buildOptions.RuntimeTypeInfo)
        //{
        //    VisualStudioUtils.WriteElementString(writer, "RuntimeTypeInfo", "true", condition);
        //}

        //if (warnings.WarningsLevel != EWarningsLevel.Default)
        //{
        //    string warnLevelString = VisualStudioUtils.GetWarningLevelString(warnings.WarningsLevel);
        //    VisualStudioUtils.WriteElementString(writer, "WarningLevel", warnLevelString, condition);
        //}

        // TODO: Support for CompileAsWinRT?
        //writer.TryWriteElementBool("CompileAsWinRT", buildOptions.CompileAsWinRT);

        //if (external.WarningsLevel != EWarningsLevel.Default)
        //{
        //    VisualStudioUtils.WriteElementString(writer, "ExternalWarningLevel", external.WarningsLevel switch
        //    {
        //        EWarningsLevel.All => "Level4",
        //        EWarningsLevel.Extra => "Level4",
        //        EWarningsLevel.On => "Level3",
        //        EWarningsLevel.Off => "TurnOffAllWarnings",
        //        _ => throw new NotImplementedException($"Unsupported External Warning Level: {external.WarningsLevel}"),
        //    }, condition);
        //}

        //VisualStudioUtils.WriteElementBool(writer, "TreatAngleIncludeAsExternal", external.AngleBrackets, condition);
    }
}

internal class ResourceFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 30;
    public override string GroupTag => "ResourceCompile";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Resource;
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        HandleExcludedFile(writer, file, projectTree, archName);
    }
}

internal class CustomBuildFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 40;
    public override string GroupTag => "CustomBuild";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Build && buildRule == EFileBuildRule.Custom;
    }

    protected override void OnWriteFile(XmlWriter writer, FileEntry file)
    {
        writer.WriteElementString("FileType", "Document");
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        if (file.BuildRule != EFileBuildRule.Custom)
        {
            return;
        }

        HandleExcludedFile(writer, file, projectTree, archName);

        BuildRuleNode buildRule = file.ProjectTree.GetMergedNode<BuildRuleNode>(file.Module.Node, (n) => n.RuleName == file.BuildRuleName, false);
        if (buildRule.RuleName != file.BuildRuleName)
        {
            throw new Exception($"No build rule with the name '{file.BuildRuleName}' was found.");
        }

        string condition = VisualStudioUtils.GetConfigCondition(projectTree, archName);

        IEnumerable<CommandNode> buildCommands = file.ProjectTree.GetNodes<CommandNode>(buildRule.Node);
        if (buildCommands.Any())
        {
            if (!string.IsNullOrEmpty(buildRule.Message))
            {
                VisualStudioUtils.WriteElementString(writer, "Message", buildRule.Message, condition);
            }

            IEnumerable<string> buildCommandStrings = buildCommands.Select((entry) => entry.GetCommandString());
            VisualStudioUtils.WriteArrayElement(writer, buildCommandStrings, "Command", null, "\r\n", condition);

            OutputsNode buildRuleOutputs = file.ProjectTree.GetMergedNode<OutputsNode>(buildRule.Node);
            IEnumerable<string> buildRuleOutputsStrings = buildRuleOutputs.Entries.Select((entry) => GetPath(entry.FilePath));
            VisualStudioUtils.WriteArrayElement(writer, buildRuleOutputsStrings, "Outputs", null, ";", condition);

            InputsNode buildRuleInputs = file.ProjectTree.GetMergedNode<InputsNode>(buildRule.Node);
            IEnumerable<string> buildRuleInputsStrings = buildRuleInputs.Entries.Select((entry) => GetPath(entry.FilePath));
            VisualStudioUtils.WriteArrayElement(writer, buildRuleInputsStrings, "AdditionalInputs", null, ";", condition);
        }

        if (buildRule.LinkOutput)
        {
            VisualStudioUtils.WriteElementString(writer, "LinkObjects", "true", condition);
        }
    }
}

internal class MidlFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 50;
    public override string GroupTag => "Midl";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Build && buildRule == EFileBuildRule.Midl;
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        if (projectTree.ProjectContext.Platform.System != EPlatformSystem.Windows)
        {
            return;
        }

        HandleExcludedFile(writer, file, projectTree, archName);
    }
}

internal class MasmFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 60;
    public override string GroupTag => "Masm";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Build && buildRule == EFileBuildRule.Asm;
    }

    protected override void OnWriteExtensionSettings(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\BuildCustomizations\\masm.props");
        writer.WriteEndElement();
    }

    protected override void OnWriteExtensionTargets(XmlWriter writer)
    {
        writer.WriteStartElement("Import");
        writer.WriteAttributeString("Project", "$(VCTargetsPath)\\BuildCustomizations\\masm.targets");
        writer.WriteEndElement();
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        HandleExcludedFile(writer, file, projectTree, archName);

        string condition = VisualStudioUtils.GetConfigCondition(projectTree, archName);

        DefinesNode defines = file.ProjectTree.GetMergedNode<DefinesNode>(file.Module.Node);
        IEnumerable<string> defineEntryStrings = defines.Entries.Select((entry) => entry.DefineName);
        VisualStudioUtils.WritePreprocessorDefinitions(writer, defineEntryStrings, false, condition);

        ExceptionsNode exceptions = file.ProjectTree.GetMergedNode<ExceptionsNode>(file.Module.Node);
        if (exceptions.ExceptionsMode == EExceptionsMode.SEH)
        {
            VisualStudioUtils.WriteElementString(writer, "UseSafeExceptionHandlers", "true", condition);
        }
    }
}

internal class ImageFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 70;
    public override string GroupTag => "Image";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Image;
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        HandleExcludedFile(writer, file, projectTree, archName);
    }
}

internal class NatvisFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 80;
    public override string GroupTag => "Natvis";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Natvis;
    }
}

internal class AppxManifestFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 90;
    public override string GroupTag => "AppxManifest";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.AppxManifest;
    }

    protected override void OnWriteFile(XmlWriter writer, FileEntry file)
    {
        writer.WriteElementString("FileType", "Document");
        writer.WriteElementString("SubType", "Designer");
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        HandleExcludedFile(writer, file, projectTree, archName);
    }
}

internal class CopyFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 100;
    public override string GroupTag => "CopyFileToFolders";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return action == EFileAction.Copy;
    }

    protected override void OnWriteFileConfig(XmlWriter writer, FileEntry file, ResolvedProjectTree projectTree, string archName)
    {
        HandleExcludedFile(writer, file, projectTree, archName);
        BuildOutputNode buildOutput = file.ProjectTree.GetMergedNode<BuildOutputNode>(file.Module.Node);
        string targetDir = file.Module.GetTargetDir(buildOutput);
        string condition = VisualStudioUtils.GetConfigCondition(projectTree, archName);

        VisualStudioUtils.WriteElementString(writer, "DestinationFolders", GetPath(targetDir), condition);
    }
}

internal class NoneFileGroup(IProjectService projectService, string vsProjectPath) : VisualStudioFileGroupBase(projectService, vsProjectPath)
{
    public override int Priority => 10000;
    public override string GroupTag => "None";

    public override bool CanHandleFile(string fullPath, EFileAction action, EFileBuildRule buildRule)
    {
        return true;
    }
}
