// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using System.Security.Cryptography;
using System.Text;
using System.Xml;

namespace Harvest.Make.Projects.ProjectGenerators.vs2026;

internal static class VisualStudioUtils
{
    private static readonly List<Type> s_fileGroupTypes = [];
    private static readonly ThreadLocal<StringBuilder> s_stringBuffers = new(() => new(1024));

    static VisualStudioUtils()
    {
        RegisterFileGroupType<ClIncludeFileGroup>();
        RegisterFileGroupType<ClCompileFileGroup>();
        RegisterFileGroupType<ResourceFileGroup>();
        RegisterFileGroupType<CustomBuildFileGroup>();
        RegisterFileGroupType<MidlFileGroup>();
        RegisterFileGroupType<MasmFileGroup>();
        RegisterFileGroupType<ImageFileGroup>();
        RegisterFileGroupType<NatvisFileGroup>();
        RegisterFileGroupType<AppxManifestFileGroup>();
        RegisterFileGroupType<CopyFileGroup>();
        RegisterFileGroupType<NoneFileGroup>();
    }

    public static List<EPlatformSystem> ValidSystems { get; } =
    [
        EPlatformSystem.DotNet,
        //EPlatformSystem.WASM, // TODO: Add support for WebAssembly built from VS
        EPlatformSystem.Windows,
    ];

    public static Dictionary<EPlatformArch, string> ArchNames { get; } = new()
    {
        { EPlatformArch.Any, "Any CPU" },
        { EPlatformArch.X86, "x86" },
        { EPlatformArch.X86_64, "x64" },
        { EPlatformArch.Arm, "ARM" },
        { EPlatformArch.Arm64, "ARM64" },
    };

    public static Dictionary<EModuleKind, string> ModuleKindNames { get; } = new()
    {
        { EModuleKind.AppConsole, "Application" },
        { EModuleKind.AppWindowed, "Application" },
        { EModuleKind.Content, "Utility" },
        { EModuleKind.Custom, "Utility" },
        { EModuleKind.LibHeader, "StaticLibrary" },
        { EModuleKind.LibStatic, "StaticLibrary" },
        { EModuleKind.LibShared, "DynamicLibrary" },
    };

    public static Dictionary<EModuleLanguage, string> LanguageToolIds { get; } = new()
    {
        { EModuleLanguage.C, "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942" },
        { EModuleLanguage.Cpp, "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942" },
        { EModuleLanguage.CSharp, "FAE04EC0-301F-11D3-BF4B-00C04F79EFBC" },
    };

    public static Dictionary<EModuleLanguage, string> LanguageProjectExtensions { get; } = new()
    {
        { EModuleLanguage.C, ".vcxproj" },
        { EModuleLanguage.Cpp, ".vcxproj" },
        { EModuleLanguage.CSharp, ".csproj" },
    };

    public static bool IsSupportedPlatform(PlatformNode platform)
    {
        return ValidSystems.Contains(platform.System) && ArchNames.ContainsKey(platform.Arch);
    }

    public static IEnumerable<(ResolvedProjectTree, string)> EnumerateConfigs(IProjectService projectService)
    {
        foreach ((_, ResolvedProjectTree project) in projectService.ResolvedProjectTrees)
        {
            PlatformNode platform = project.ProjectContext.Platform;

            if (!ValidSystems.Contains(platform.System))
            {
                continue;
            }

            if (!ArchNames.TryGetValue(platform.Arch, out string? archName))
            {
                continue;
            }

            yield return (project, archName);
        }
    }

    // Returns the path relative to projectFilePath and replaces '/' with '\\'
    public static string TranslatePath(string projectFilePath, string path)
    {
        if (path.Contains("$(") || path.Contains("%("))
        {
            return path.Replace('/', '\\');
        }

        string projectDir = Path.GetDirectoryName(projectFilePath) ?? Directory.GetCurrentDirectory();
        return Path.GetRelativePath(projectDir, path).Replace('/', '\\');
    }

    public static void WriteElementString(XmlWriter writer, string elementName, string value, string condition)
    {
        writer.WriteStartElement(elementName);
        if (!string.IsNullOrEmpty(condition))
        {
            writer.WriteAttributeString("Condition", condition);
        }
        writer.WriteString(value);
        writer.WriteEndElement();
    }

    public static void WriteElementBool(XmlWriter writer, string elementName, bool value, string condition)
    {
        writer.WriteStartElement(elementName);
        if (!string.IsNullOrEmpty(condition))
        {
            writer.WriteAttributeString("Condition", condition);
        }
        writer.WriteString(value ? "true" : "false");
        writer.WriteEndElement();
    }

    public static string GetArrayElementValue(IEnumerable<string> values, string? additionalName = null, string separator = ";")
    {
        if (s_stringBuffers.Value is not StringBuilder stringBuffer)
        {
            return "";
        }

        stringBuffer.Clear();
        stringBuffer.AppendJoin(separator, values);

        if (stringBuffer.Length > 0 && !string.IsNullOrEmpty(additionalName))
        {
            stringBuffer.Append($"{separator}{additionalName}");
        }

        return stringBuffer.ToString();
    }

    public static void WriteArrayElement(XmlWriter writer, IEnumerable<string> values, string elementName, string? additionalName = null, string separator = ";", string? condition = null)
    {
        string value = GetArrayElementValue(values, additionalName, separator);
        if (!string.IsNullOrEmpty(value))
        {
            writer.WriteStartElement(elementName);
            if (!string.IsNullOrEmpty(condition))
            {
                writer.WriteAttributeString("Condition", condition);
            }
            writer.WriteString(value);
            writer.WriteEndElement();
        }
    }

    public static void WritePreprocessorDefinitions(XmlWriter writer, IEnumerable<string> defines, bool escapeQuotes, string? condition = null)
    {
        IEnumerable<string> defineEntryStrings = defines.Select((entry) => escapeQuotes ? entry.Replace("\"", "\\\"") : entry);
        string value = GetArrayElementValue(defineEntryStrings, "%(PreprocessorDefinitions)");
        if (!string.IsNullOrEmpty(value))
        {
            writer.WriteStartElement("PreprocessorDefinitions");
            if (!string.IsNullOrEmpty(condition))
            {
                writer.WriteAttributeString("Condition", condition);
            }
            writer.WriteString(value);
            writer.WriteEndElement();
        }
    }

    public static void WriteAdditionalIncludeDirs(XmlWriter writer, IEnumerable<string> includeDirs)
    {
        WriteArrayElement(writer, includeDirs, "AdditionalIncludeDirectories", "%(AdditionalIncludeDirectories)");
    }

    public static bool IsManaged(ModuleNode module, BuildOptionsNode buildOptions)
    {
        return module.Language == EModuleLanguage.CSharp || buildOptions.ClrMode != EBuildClrMode.Off;
    }

    public static bool IsClrMixed(ModuleNode module, BuildOptionsNode buildOptions)
    {
        bool isMixed = false;

        if (buildOptions.ClrMode != EBuildClrMode.Off)
        {
            return isMixed;
        }

        // TODO: If/when we add file-specific configuration, need to check each file in the module
        // and if any file is managed then mixed = true.

        return isMixed;
    }

    public static bool IsOptimizedBuild(OptimizeNode optimize)
    {
        return optimize.OptimizationLevel != EOptimizationLevel.Off && optimize.OptimizationLevel != EOptimizationLevel.Debug;
    }

    public static bool IsDebugBuild(OptimizeNode optimize, SymbolsNode symbols)
    {
        return symbols.SymbolsMode == ESymbolsMode.On
            && !IsOptimizedBuild(optimize);
    }

    public static bool CanLinkIncrememntal(ModuleNode module, LinkOptionsNode linkOptions, OptimizeNode optimize)
    {
        return module.Kind != EModuleKind.LibStatic
            && linkOptions.IncrementalLink
            && optimize.LinkTimeOptimizationLevel != ELinkTimeOptimizationLevel.On
            && !IsOptimizedBuild(optimize);
    }

    public static string GetConfigName(ResolvedProjectTree project)
    {
        return GetConfigName(project.ProjectContext.Configuration, project.ProjectContext.Platform);
    }

    public static string GetConfigName(ResolvedProjectTree project, string archName)
    {
        return GetConfigName(project.ProjectContext.Configuration, project.ProjectContext.Platform, archName);
    }

    public static string GetConfigName(ConfigurationNode configuration, PlatformNode platform)
    {
        return $"{configuration.ConfigName} {platform.PlatformName}";
    }

    public static string GetConfigName(ConfigurationNode configuration, PlatformNode platform, string archName)
    {
        string baseConfigName = GetConfigName(configuration, platform);
        return $"{baseConfigName}|{archName}";
    }

    public static string GetConfigCondition(ConfigurationNode configuration, PlatformNode platform, string archName)
    {
        string configKeyName = GetConfigName(configuration, platform, archName);
        return $"'$(Configuration)|$(Platform)'=='{configKeyName}'";
    }

    public static string GetConfigCondition(ResolvedProjectTree project, string archName)
    {
        return GetConfigCondition(project.ProjectContext.Configuration, project.ProjectContext.Platform, archName);
    }

    public static string GetWarningLevelString(EWarningsLevel level)
    {
        return level switch
        {
            EWarningsLevel.Default => "Level3",
            EWarningsLevel.All => "EnableAllWarnings",
            EWarningsLevel.Extra => "Level4",
            EWarningsLevel.On => "Level3",
            EWarningsLevel.Off => "TurnOffAllWarnings",
            _ => throw new NotImplementedException($"Unsupported warning level: {level}")
        };
    }

    public static string EnsureLibraryExtension(string path)
    {
        string ext = Path.GetExtension(path);
        if (ext == ".lib" || ext == ".obj")
        {
            return path;
        }
        return path + ".lib";
    }

    public static void RegisterFileGroupType<T>() where T : IVisualStudioFileGroup
    {
        if (s_fileGroupTypes.Contains(typeof(T)))
        {
            throw new InvalidOperationException($"File group type {typeof(T).Name} is already registered.");
        }

        s_fileGroupTypes.Add(typeof(T));
    }

    public static List<Type> GetFileGroupTypes()
    {
        return s_fileGroupTypes;
    }

    public static List<IVisualStudioFileGroup> CreateFileGroups(IProjectService projectService, string vsProjectPath)
    {
        List<IVisualStudioFileGroup> groups = [];

        foreach (Type type in s_fileGroupTypes)
        {
            if (Activator.CreateInstance(type, projectService, vsProjectPath) is IVisualStudioFileGroup group)
            {
                groups.Add(group);
            }
            else
            {
                throw new InvalidOperationException($"Failed to create instance of file group type {type.Name}.");
            }
        }

        groups.Sort((a, b) =>
        {
            if (a.Priority < b.Priority)
            {
                return -1;
            }

            if (a.Priority > b.Priority)
            {
                return 1;
            }

            return 0;
        });

        return groups;
    }


    public static Guid CreateGuidFromString(string name, Guid namespaceGuid)
    {
        using IncrementalHash hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA1);
        hash.AppendData(namespaceGuid.ToByteArray());
        hash.AppendData(Encoding.UTF8.GetBytes(name));

        byte[] result = hash.GetHashAndReset();

        //set high-nibble to 5 to indicate type 5
        result[6] &= 0x0F;
        result[6] |= 0x50;

        //set upper two bits to "10"
        result[8] &= 0x3F;
        result[8] |= 0x80;

        return new Guid(result.AsSpan(0, 16));
    }

    private static readonly Guid s_filtersNamespaceGuid = new("CFCAC646-520C-4F5C-995E-BA007DEA1508");

    public static Guid CreateGuidForFilter(string name)
    {
        return CreateGuidFromString(name, s_filtersNamespaceGuid);
    }
}
