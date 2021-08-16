namespace he.schema.test;

/// Unique identifier for an asset within the project. This is an RFC4122 v4 UUID.
struct AssetId { bytes: uint8[16]; }

/// Unique identifier for an asset within the project. This is an RFC4122 v4 UUID.
struct AssetFileId { bytes: uint8[16]; }

/// State of an asset in the system.
enum AssetState : uint8
{
    /// State is unknown. This is likely a new asset.
    Unknown = 0,

    /// An import is pending. This usually means the asset was queued for processing but did not
    /// finish before the application was closed.
    PendingImport = 1,

    /// A compile is pending. This usually means the asset was queued for processing but did not
    /// finish before the application was closed.
    PendingCompile = 2,


    /// An unknown failure was encountered while processing the asset. This should never happen.
    Failed = 100,

    /// The import process failed. There should be messages in the message table
    /// that express more details about the failure.
    FailedImport = 101,

    /// The compile process failed. There should be messages in the message table
    /// that express more details about the failure.
    FailedCompile = 102,


    /// The asset has been fully processed and is ready for use in the game.
    Ready = 255,
}

/// A structure representing a single asset in the system.
struct Asset
{
    /// Unique identifier for the asset.
    id: AssetId;

    /// User-defined string name of the asset.
    name: string;

    /// Unique identifier for the asset type. This is an FNV-32 hash of the type name.
    type: uint32;

    /// User-defined tags the asset can be searched by.
    tags: string[];

    /// Import settings owned by the importer used to create this asset. The importer can set any
    /// arbitrary bytes here it wishes. It is recommended to store here only the minimum
    /// information needed to replicate the import. This often means user settings and not
    /// intermediate data that can be regenerated from the source data.
    /// Intermediate data should instead be stored as a Resource since it can be regenerated, or
    /// downloaded from a shared cache, at any time.
    importer_settings: uint8[];
}

struct AssetFile
{
    /// Unique identifier for the asset file.
    id: AssetFileId;

    /// The identifier for the importer that processes the source. This is an FNV-32 hash of the type's FQN.
    /// A value of zero indicates that these assets were not created through an import process.
    importer_id: uint32;

    /// The version of the importer that processes the source.
    /// If the importer in the built application differs from this version the asset will be reimported.
    importer_version: uint64;

    /// Array of assets that live in this file. Usually there is only one of these per AssetFile,
    /// however there can be more. For example, importing an FBX may result in many assets.
    assets: Asset[];
}
