// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects;

public static class NodeTokenResolvers
{
    public static bool ConfiurationTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value)
    {
        Debug.Assert(contextName == ConfigurationNode.NodeTraits.Name);

        switch (propertyName)
        {
            case "name":
            {
                value = projectContext.Configuration?.ConfigName ?? "";
                return true;
            }
        }

        value = default;
        return false;
    }

    public static bool PlatformTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value)
    {
        Debug.Assert(contextName == PlatformNode.NodeTraits.Name);

        switch (propertyName)
        {
            case "name":
            {
                value = projectContext.Platform?.PlatformName ?? "";
                return true;
            }
        }

        value = default;
        return false;
    }

    public static bool ProjectTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value)
    {
        Debug.Assert(contextName == ProjectNode.NodeTraits.Name);

        switch (propertyName)
        {
            case "name":
            {
                value = projectContext.ProjectService.ProjectNode.ProjectName;
                return true;
            }

            case "path":
            {
                value = projectContext.ProjectService.ProjectPath;
                return true;
            }
        }

        value = default;
        return false;
    }

    public static bool PluginTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value)
    {
        Debug.Assert(contextName == PluginNode.NodeTraits.Name);

        if (projectContext.Plugin is null)
        {
            value = default;
            return false;
        }

        switch (propertyName)
        {
            case "name":
            {
                value = projectContext.Plugin.PluginName;
                return true;
            }
            case "path":
            {
                value = projectContext.Plugin.Node.SourceInfo.FilePath;
                return true;
            }
            case "install_dir":
            {
                value = projectContext.Plugin.GetInstallDir(projectContext);
                return true;
            }
        }

        value = default;
        return false;
    }

    public static bool ModuleTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value)
    {
        Debug.Assert(contextName == ModuleNode.NodeTraits.Name);

        if (projectContext.Module is null)
        {
            value = default;
            return false;
        }

        switch (propertyName)
        {
            case "name":
            {
                value = projectContext.Module.ModuleName;
                return true;
            }
            case "path":
            {
                value = projectContext.Module.Node.SourceInfo.FilePath;
                return true;
            }
            case "build_target":
            {
                string targetDir = projectContext.Module.GetTargetDir(projectContext);
                string targetName = projectContext.Module.TargetName;
                string targetExtension = projectContext.Module.GetTargetExtension(projectContext);
                value = Path.Join(targetDir, targetName + targetExtension);
                return true;
            }
            case "link_target":
            {
                // Treat shared libraries as static libraries for the purpose of linking. This will let us target
                // the import library (.lib) instead of the shared library (.dll).
                EModuleKind moduleKind = projectContext.IsWindows && projectContext.Module.MakeImportLib && projectContext.Module.Kind == EModuleKind.LibShared ? EModuleKind.LibStatic : projectContext.Module.Kind;
                string targetDir = projectContext.Module.GetTargetDir(projectContext, moduleKind);
                string targetName = projectContext.Module.TargetName;
                string targetExtension = projectContext.Module.GetTargetExtension(projectContext);
                value = Path.Join(targetDir, targetName + targetExtension);
                return true;
            }
            case "gen_dir":
            {
                value = projectContext.Module.GetGenDir(projectContext);
                return true;
            }
        }

        value = default;
        return false;
    }
}
