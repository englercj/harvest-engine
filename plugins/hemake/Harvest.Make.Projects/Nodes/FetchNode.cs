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
}

public class FetchNode(KdlNode node) : NodeBase<FetchNodeTraits>(node)
{
    public EFetchMethod Method => GetEnumValue<EFetchMethod>(0);

    public int InstallDirPriority => GetNumberValue<int>("install_dir_priority");

    public string BitBucketUser => Method == EFetchMethod.BitBucket ? GetStringValue("user") : string.Empty;
    public string BitBucketRepo => Method == EFetchMethod.BitBucket ? GetStringValue("repo") : string.Empty;
    public string BitBucketRef => Method == EFetchMethod.BitBucket ? GetStringValue("ref") : string.Empty;

    public string GitHubUser => Method == EFetchMethod.GitHub ? GetStringValue("user") : string.Empty;
    public string GitHubRepo => Method == EFetchMethod.GitHub ? GetStringValue("repo") : string.Empty;
    public string GitHubRef => Method == EFetchMethod.GitHub ? GetStringValue("ref") : string.Empty;

    public string NugetPackage => Method == EFetchMethod.Nuget ? GetStringValue("package") : string.Empty;
    public string NugetVersion => Method == EFetchMethod.Nuget ? GetStringValue("version") : string.Empty;

    private string? _archiveUrl = null;
    public string ArchiveUrl => _archiveUrl ??= Method switch
    {
        EFetchMethod.Archive => GetStringValue("url"),
        EFetchMethod.BitBucket => $"https://bitbucket.org/{BitBucketUser}/{BitBucketRepo}/get/{BitBucketRef}.zip",
        EFetchMethod.GitHub => $"https://github.com/{GitHubUser}/{GitHubRepo}/archive/{GitHubRef}.zip",
        EFetchMethod.Nuget => $"https://www.nuget.org/api/v2/package/{NugetPackage}/{NugetVersion}",
        _ => string.Empty,
    };

    private string? _archiveKey = null;
    public string ArchiveKey => _archiveKey ??= ArchiveUrl.ToSHA256HexDigest();

    private EFetchArchiveFormat? _archiveFormat = null;
    public EFetchArchiveFormat ArchiveFormat => _archiveFormat ??= GetArchiveFormat();

    private string? _archiveBaseDir = null;
    public string ArchiveBaseDir => _archiveBaseDir ??= GetArchiveBaseDir();

    // Manually validate required properties based on the fetch method. We need to do this in a
    // method override because there isn't currently a way to express conditionally required
    // properties in the NodePropertyDefs list.
    protected override void ValidateProperties()
    {
        // Note: Argument validation is handled by the base NodeBase class and should have already
        // occurred before this method is called, so we can safely rely on the Method property.

        switch (Method)
        {
            case EFetchMethod.Archive:
            {
                RequireProperty("url");
                break;
            }
            case EFetchMethod.BitBucket:
            case EFetchMethod.GitHub:
            {
                RequireProperty("user");
                RequireProperty("repo");
                RequireProperty("ref");
                break;
            }
            case EFetchMethod.Nuget:
            {
                RequireProperty("package");
                RequireProperty("version");
                break;
            }
        }

        // Validate the value types of specified properties
        base.ValidateProperties();
    }

    private void RequireProperty(string propName)
    {
        if (!Node.Properties.ContainsKey(propName))
        {
            throw new NodeValidationException(this, $"'{Node.Name}' nodes with method '{KdlEnumUtils.GetName(Method)}' must specify a '{propName}' property.");
        }
    }

    private EFetchArchiveFormat GetArchiveFormat()
    {
        if (Method == EFetchMethod.Archive)
        {
            if (TryGetEnumValue<EFetchArchiveFormat>("archive_format") is EFetchArchiveFormat specifiedType)
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
        if (TryGetStringValue("base_dir") is string specifiedBaseDir)
        {
            return specifiedBaseDir;
        }

        switch (Method)
        {
            // Archive and Nuget use the root of the archive unless otherwise specified
            case EFetchMethod.Archive:
            case EFetchMethod.Nuget:
            {
                return string.Empty;
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

        return string.Empty;
    }
}
