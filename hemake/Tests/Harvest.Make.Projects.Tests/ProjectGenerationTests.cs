// Copyright Chad Engler

using System.Xml.Linq;

namespace Harvest.Make.Projects.Tests;

public sealed class ProjectGenerationTests(ProjectGenerationFixture fixture) : IClassFixture<ProjectGenerationFixture>
{
    [Fact]
    public void GeneratesSlnxWithExpectedProjectsAndBuildMappings()
    {
        Assert.True(File.Exists(fixture.SlnxPath));

        XDocument slnx = XDocument.Parse(fixture.SlnxText);
        XElement root = Assert.IsType<XElement>(slnx.Root);
        Assert.Equal("Solution", root.Name.LocalName);

        List<XElement> projects = root.Descendants().Where((n) => n.Name.LocalName == "Project").ToList();
        Assert.Contains(projects, (p) => p.Attribute("Path")?.Value.Contains("test_app.vcxproj", StringComparison.OrdinalIgnoreCase) == true);
        Assert.Contains(projects, (p) => p.Attribute("Path")?.Value.Contains("test_lib.vcxproj", StringComparison.OrdinalIgnoreCase) == true);
        Assert.Contains(projects, (p) => p.Attribute("Path")?.Value.Contains("test_tool.vcxproj", StringComparison.OrdinalIgnoreCase) == true);

        XElement appProject = projects.Single((p) => p.Attribute("Path")?.Value.Contains("test_app.vcxproj", StringComparison.OrdinalIgnoreCase) == true);
        IEnumerable<XElement> appBuildTypes = appProject.Elements().Where((n) => n.Name.LocalName == "BuildType");
        Assert.Contains(appBuildTypes, (n) => n.Attribute("Solution")?.Value == "Debug|Win64" && n.Attribute("Project")?.Value == "Debug Win64");

        IEnumerable<XElement> appPlatforms = appProject.Elements().Where((n) => n.Name.LocalName == "Platform");
        Assert.Contains(appPlatforms, (n) => n.Attribute("Solution")?.Value == "*|Win64" && n.Attribute("Project")?.Value == "x64");
    }

    [Fact]
    public void GeneratesAppVcxprojWithExpectedCompilerAndLinkerData()
    {
        Assert.True(File.Exists(fixture.AppVcxprojPath));

        Assert.Contains("ProjectGuid", fixture.AppVcxprojText);
        Assert.Contains("ProjectReference Include=\"test_lib.vcxproj\"", fixture.AppVcxprojText);
        Assert.Contains("PrecompiledHeaderFile", fixture.AppVcxprojText);
        Assert.Contains("FEATURE_ENABLED", fixture.AppVcxprojText);
        Assert.Contains("FROM_WHEN_DEBUG", fixture.AppVcxprojText);
        Assert.Contains("AdditionalIncludeDirectories", fixture.AppVcxprojText);
        Assert.Contains("AdditionalLibraryDirectories", fixture.AppVcxprojText);
        Assert.Contains("LanguageStandard", fixture.AppVcxprojText);
        Assert.Contains("EnableEnhancedInstructionSet", fixture.AppVcxprojText);
        Assert.Contains("EnableASAN", fixture.AppVcxprojText);
        Assert.Contains("PreBuildEvent", fixture.AppVcxprojText);
        Assert.Contains("CustomBuild", fixture.AppVcxprojText);
        Assert.Contains("Natvis", fixture.AppVcxprojText);
        Assert.Contains("CopyFileToFolders", fixture.AppVcxprojText);
    }

    [Fact]
    public void GeneratesAdditionalModuleProjects()
    {
        Assert.True(File.Exists(fixture.LibVcxprojPath));
        Assert.True(File.Exists(fixture.ToolVcxprojPath));

        Assert.Contains("StaticLibrary", fixture.LibVcxprojText);
        Assert.Contains("Utility", fixture.ToolVcxprojText);
    }
}
