// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;
using Harvest.Make.Utils;

namespace Harvest.Make.Projects.Nodes;

public enum EFetchMethod
{
    [KdlName("archive")] Archive,
    [KdlName("bitbucket")] BitBucket,
    [KdlName("github")] GitHub,
    [KdlName("nuget")] Nuget,
}

public enum EFetchArchiveFormat
{
    [KdlName("zip")] Zip,
    [KdlName("tar")] Tar,
    [KdlName("tar.gz")] TarGz,
}

public class FetchNodeTraits : NodeBaseTraits
{
    public override string Name => "fetch";

    public override IReadOnlyList<string> ValidScopes =>
    [
        InstallNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EFetchMethod>.Required(EFetchMethod.Archive),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        // shared by all methods
        { "base_dir", NodeValueDef_Path.Optional() },
        { "install_dir_priority", NodeValueDef_Number<int>.Optional(0) },

        // Archive
        { "url", NodeValueDef_String.Optional() },
        { "archive_format", NodeValueDef_Enum<EFetchArchiveFormat>.Optional() },

        // BitBucket / Github
        { "user", NodeValueDef_String.Optional() },
        { "repo", NodeValueDef_String.Optional() },
        { "ref", NodeValueDef_String.Optional() },

        // Nuget
        { "package", NodeValueDef_String.Optional() },
        { "version", NodeValueDef_String.Optional() },
    };

    protected override void ValidateProperties(KdlNode node)
    {
        // Manually validate required properties based on the fetch method. We need to do this in a
        // method override because there isn't currently a way to express conditionally required
        // properties in the NodePropertyDefs list.

        // Note: Argument validation is handled by the base NodeBase class and should have already
        // occurred before this method is called, so we can safely query the method.
        EFetchMethod method = GetEnumValue<EFetchMethod>(node, 0);
        switch (method)
        {
            case EFetchMethod.Archive:
            {
                RequireProperty(node, "url");
                break;
            }
            case EFetchMethod.BitBucket:
            case EFetchMethod.GitHub:
            {
                RequireProperty(node, "user");
                RequireProperty(node, "repo");
                RequireProperty(node, "ref");
                break;
            }
            case EFetchMethod.Nuget:
            {
                RequireProperty(node, "package");
                RequireProperty(node, "version");
                break;
            }
        }

        // Validate the value types of specified properties
        base.ValidateProperties(node);
    }

    private static void RequireProperty(KdlNode node, string propName)
    {
        if (!node.HasValue(propName))
        {
            throw new NodeParseException(node, $"'{node.Name}' nodes with method '{node.Arguments[0].GetValueString()}' must specify a '{propName}' property.");
        }
    }

    public override INode CreateNode(KdlNode node) => new FetchNode(node);
}

public class FetchNode(KdlNode node) : NodeBase<FetchNodeTraits>(node)
{
    public EFetchMethod Method => GetEnumValue<EFetchMethod>(0);

    public int InstallDirPriority => GetValue<int>("install_dir_priority");

    public string BitBucketUser => Method == EFetchMethod.BitBucket ? GetValue<string>("user") : "";
    public string BitBucketRepo => Method == EFetchMethod.BitBucket ? GetValue<string>("repo") : "";
    public string BitBucketRef => Method == EFetchMethod.BitBucket ? GetValue<string>("ref") : "";

    public string GitHubUser => Method == EFetchMethod.GitHub ? GetValue<string>("user") : "";
    public string GitHubRepo => Method == EFetchMethod.GitHub ? GetValue<string>("repo") : "";
    public string GitHubRef => Method == EFetchMethod.GitHub ? GetValue<string>("ref") : "";

    public string NugetPackage => Method == EFetchMethod.Nuget ? GetValue<string>("package") : "";
    public string NugetVersion => Method == EFetchMethod.Nuget ? GetValue<string>("version") : "";

    public string ArchiveUrl => Method switch
    {
        EFetchMethod.Archive => GetValue<string>("url"),
        EFetchMethod.BitBucket => $"https://bitbucket.org/{BitBucketUser}/{BitBucketRepo}/get/{BitBucketRef}.zip",
        EFetchMethod.GitHub => $"https://github.com/{GitHubUser}/{GitHubRepo}/archive/{GitHubRef}.zip",
        EFetchMethod.Nuget => $"https://www.nuget.org/api/v2/package/{NugetPackage}/{NugetVersion}",
        _ => "",
    };

    private string? _archiveKey = null;
    public string ArchiveKey => _archiveKey ??= ArchiveUrl.GetSHA256Hash().ToString();

    public EFetchArchiveFormat ArchiveFormat => GetArchiveFormat();

    public string ArchiveBaseDir => GetArchiveBaseDir();

    private EFetchArchiveFormat GetArchiveFormat()
    {
        if (Method == EFetchMethod.Archive)
        {
            if (TryGetEnumValue("archive_format", out EFetchArchiveFormat specifiedType))
            {
                return specifiedType;
            }

            // Try to infer from the URL file extension
            string urlPath = new Uri(ArchiveUrl).LocalPath.ToLowerInvariant();
            if (urlPath.EndsWith(".zip"))
            {
                return EFetchArchiveFormat.Zip;
            }
            else if (urlPath.EndsWith(".tar"))
            {
                return EFetchArchiveFormat.Tar;
            }
            else if (urlPath.EndsWith(".tar.gz") || urlPath.EndsWith(".tgz"))
            {
                return EFetchArchiveFormat.TarGz;
            }

            // Fall through to a default of zip if we can't determine the type
        }

        // All other fetch methods use zip archives
        return EFetchArchiveFormat.Zip;
    }

    private string GetArchiveBaseDir()
    {
        if (TryGetValue<string>("base_dir", out string? specifiedBaseDir))
        {
            return specifiedBaseDir;
        }

        switch (Method)
        {
            // Archive and Nuget use the root of the archive unless otherwise specified
            case EFetchMethod.Archive:
            case EFetchMethod.Nuget:
            {
                return "";
            }
            case EFetchMethod.BitBucket:
            {
                // TODO: Bitbucket format is <user>-<repo>-git-<short_sha>, but since the ref can
                // be a branch or tag name we don't necessarily know the short sha here.
                // In lua I used to query the filesystem to get the actual extracted dir name,
                // maybe do that here too?
                // The HTTP response from Bitbucket includes a Content-Disposition header with
                // the actual filename, maybe we can do something with that at install time?
                string trimmedRef = BitBucketRef.StartsWith('v') ? BitBucketRef[1..] : BitBucketRef;
                return $"{BitBucketUser}-{BitBucketRepo}-git-{trimmedRef}";
            }
            case EFetchMethod.GitHub:
            {
                string trimmedRef = GitHubRef.StartsWith('v') ? GitHubRef[1..] : GitHubRef;
                return $"{GitHubRepo}-{trimmedRef}";
            }
        };

        return "";
    }
}
