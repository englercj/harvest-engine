// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Xml;

namespace Harvest.Make.Projects.Generators.vs2022;

public static class VisualStudioUtils
{
    public static List<EPlatformSystem> ValidSystems { get; } =
    [
        EPlatformSystem.DotNet,
        EPlatformSystem.WASM,
        EPlatformSystem.Windows
    ];

    public static Dictionary<EPlatformArch, string> ArchNames { get; } = new()
    {
        { EPlatformArch.Any, "Any CPU" },
        { EPlatformArch.X86, "x86" },
        { EPlatformArch.X86_64, "x64" },
        { EPlatformArch.Arm, "ARM" },
        { EPlatformArch.Arm64, "ARM64" }
    };

    public static Dictionary<EModuleKind, string> ModuleKindNames { get; } = new()
    {
        { EModuleKind.AppConsole, "Application" },
        { EModuleKind.AppWindowed, "Application" },
        { EModuleKind.Content, "Utility" },
        { EModuleKind.Custom, "Utility" },
        { EModuleKind.LibHeader, "DynamicLibrary" },
        { EModuleKind.LibStatic, "StaticLibrary" },
        { EModuleKind.LibShared, "DynamicLibrary" },
    };

    public static Dictionary<EModuleLanguage, string> LanguageToolIds { get; } = new()
    {
        { EModuleLanguage.C, "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942" },
        { EModuleLanguage.Cpp, "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942" },
        { EModuleLanguage.CSharp, "FAE04EC0-301F-11D3-BF4B-00C04F79EFBC" },
        { EModuleLanguage.FSharp, "F2A71F9B-5D33-465A-A702-920D77279786" },
    };

    public static Dictionary<EModuleLanguage, string> LanguageProjectExtensions { get; } = new()
    {
        { EModuleLanguage.C, ".vcxproj" },
        { EModuleLanguage.Cpp, ".vcxproj" },
        { EModuleLanguage.CSharp, ".csproj" },
        { EModuleLanguage.FSharp, ".fsproj" },
    };

    public static void ForEachConfig(ProjectGeneratorHelper helper, Action<ConfigurationNode, PlatformNode, string> action)
    {
        helper.ForEachConfig((ConfigurationNode configuration, PlatformNode platform) =>
        {
            if (!ValidSystems.Contains(platform.System))
                return;

            if (!ArchNames.TryGetValue(platform.Arch, out string? archName))
                return;

            action(configuration, platform, archName);
        });
    }

    public static bool IsManaged(ModuleNode module)
    {
        return module.Language == EModuleLanguage.CSharp || module.ClrMode != EModuleClrMode.Off;
    }

    public static bool IsClrMixed(ModuleNode module)
    {
        throw new NotImplementedException();
    }
}

public interface IFileBuildInfo
{
    public int Priority { get; }

    public void EmitFiles(XmlWriter writer, List<string> paths);
    public void EmitFilter(XmlWriter writer);
    public void EmitExtensionProps(XmlWriter writer);
    public void EmitExtensionTargets(XmlWriter writer);
}

public abstract class BaseFileBuildInfo : IFileBuildInfo
{
    public abstract int Priority { get; }

    public abstract void EmitFiles(XmlWriter writer, List<string> paths);
    public abstract void EmitFilter(XmlWriter writer);
    public abstract void EmitExtensionProps(XmlWriter writer);
    public abstract void EmitExtensionTargets(XmlWriter writer);

    protected void EmitFilesInternal(
        XmlWriter writer,
        ProjectGeneratorHelper helper,
        string tag,
        List<string> paths,
        Action<XmlWriter, string> fileAction,
        Func<string, ConfigurationNode, PlatformNode, bool> checkFunc)
    {
        if (paths.Count == 0)
            return;

        writer.WriteStartElement("ItemGroup");

        foreach (string path in paths)
        {
            writer.WriteStartElement(tag);
            writer.WriteAttributeString("Include", path); // TODO: need to use file's relative path to project & translate to use VS tokens

            fileAction(path);
            helper.ForEachConfig((ConfigurationNode config, PlatformNode platform) =>
            {
                if (checkFunc(path, config, platform))
                {
                    //writer.WriteStartElement("Configuration");
                    //writer.WriteAttributeString("Include", $"{config.Name}|{platform.Name}");
                    //writer.WriteEndElement();
                }
            });

            writer.WriteEndElement();
        }

        writer.WriteEndElement();
    }
}

public class ClIncludeBuildInfo(ProjectGeneratorHelper helper) : BaseFileBuildInfo
{
    public override int Priority => 1;

    private readonly ProjectGeneratorHelper _helper = helper;

    public override void EmitFiles(XmlWriter writer, List<string> paths)
    {
        EmitFilesInternal(writer, _helper, "ClInclude", paths, EmitFileAction, (path, config, platform) => true);
    }

    public override void EmitFilter(XmlWriter writer)
    {

    }

    public override void EmitExtensionProps(XmlWriter writer)
    {
    }

    public override void EmitExtensionTargets(XmlWriter writer)
    {
    }

    private void EmitFileAction(XmlWriter writer, string path)
    {
        if (path.IsGenerated)
        {
            string path = path.translate(File.dependsOn.relpath);
            writer.WriteElementString("AutoGen", "true");
            writer.WriteElementString("DependentUpon", path);
        }
    }
}
