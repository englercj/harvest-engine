// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/concepts.h"
#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/name.h"
#include "he/core/string.h"
#include "he/core/string_builder.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/core/uuid.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/orm_storage.h"

namespace he::assets
{
    class AssetDatabase;

    enum class AssetState : uint8_t
    {
        Unknown = 0,
        NeedsImport = 1,
        NeedsCompile = 2,
        Ready = 3,

        // Failure states
        ImportFailed = 4,
        CompileFailed = 5,
    };

    struct AssetFileModel final
    {
        /// RowID of the model in the database.
        uint32_t id{ 0 };

        /// Unique ID of the asset file.
        AssetFileUuid uuid{};

        /// Asset-root-relative path to the asset file on disk.
        String filePath{};

        /// Counter for the number of path components in the asset-root-relative path to this file.
        /// This is used to quickly perform searches of assets within particular directories.
        uint32_t filePathDepth{ 0 };

        /// The last-modified time of the asset file when it was last read from disk. Used to
        /// detect changes to the file that happened while the Harvest Editor was closed.
        SystemTime fileWriteTime{ 0 };

        /// The byte size of the asset file when it was last read from disk. Used to
        /// detect changes to the file that happened while the Harvest Editor was closed.
        uint64_t fileSize{ 0 };

        /// Asset-root-relative path to the source data file on disk.
        /// The source data file is an arbitrary file that is associated with the assets in this
        /// asset file. Usually, this is the source file that assets are imported from.
        String sourcePath{};

        /// The last-modified time of the source file when it was last read from disk. Used to
        /// detect changes to the file that happened while the Harvest Editor was closed.
        SystemTime sourceWriteTime{ 0 };

        /// The byte size of the source file when it was last read from disk. Used to
        /// detect changes to the file that happened while the Harvest Editor was closed.
        uint64_t sourceSize{ 0 };

        /// A token value representing the last scanner to update this row in the database.
        /// This is used to detect files listed in the database that are missing from disk.
        uint32_t scanToken{ 0 };

        static bool AddOrUpdate(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model);
    };

    struct AssetModel final
    {
        /// RowID of the model in the database.
        uint32_t id{ 0 };

        /// Unique ID of the asset.
        AssetUuid uuid{};

        /// RowID of the asset file this asset is contained within.
        uint32_t assetFileId{ 0 };

        /// The asset type of the asset.
        Name assetType{};

        /// User-defined string name for the asset. This often matches the file name, but is
        /// not required to.
        String name{};

        /// State of the asset in the processing pipeline.
        AssetState state{ AssetState::Unknown };

        /// A crc32c hash of the asset's data that is stored in the asset file.
        uint32_t dataHash{ 0 };

        /// A crc32c hash of the asset's import data that is stored in the asset file.
        uint32_t importDataHash{ 0 };

        /// The ID of the importer which processed this asset.
        uint32_t importerId{ 0 };

        /// The version of the importer which processed this asset.
        uint32_t importerVersion{ 0 };

        /// The ID of the compiler which processed this asset.
        uint32_t compilerId{ 0 };

        /// The version of the compiler which processed this asset.
        uint32_t compilerVersion{ 0 };

        static bool FindAllByAssetFilePath(AssetDatabase& db, Vector<AssetModel>& models, StringView pathPrefix);
    };

    struct ConfigModel final
    {
        /// RowID of the model in the database.
        uint32_t id{ 0 };

        // The unique key of this config.
        String key{};

        /// The byte data of the config value.
        Vector<uint8_t> value{};

        template <TriviallyCopyable T>
        void SetValue(const T& v)
        {
            value.Resize(sizeof(T), DefaultInit);
            MemCopy(value.Data(), &v, sizeof(T));
        }
    };

    struct TagModel final
    {
        /// RowID of the model in the database.
        uint32_t id{ 0 };

        /// The text content of the tag entry.
        String name{};
    };

    struct AssetTagModel final
    {
        /// RowID of the model in the database.
        uint32_t id{ 0 };

        /// RowID of the asset this association references.
        uint32_t assetId{ 0 };

        /// RowID of the tag this association references.
        uint32_t tagId{ 0 };

        static bool Add(AssetDatabase& db, const AssetUuid& assetUuid, StringView tag);
        static bool Remove(AssetDatabase& db, const AssetUuid& assetUuid, StringView tag);
        static bool RemoveAll(AssetDatabase& db, const AssetUuid& assetUuid);
    };

    struct AssetReferenceModel final
    {
        /// RowID of the model in the database.
        uint32_t id{ 0 };

        /// Source asset the reference is from.
        uint32_t fromAssetId{ 0 };

        /// Destination asset the reference is to.
        /// Only valid when `kind` is `Asset` or `Resource`.
        uint32_t toAssetId{ 0 };
    };

    constexpr auto AssetDbSchema = sqlite::DefineSchema(
        sqlite::Table("asset_file",
            sqlite::Column("id", &AssetFileModel::id, sqlite::PrimaryKey()),
            sqlite::Column("uuid", &AssetFileModel::uuid, sqlite::Unique(), sqlite::NotNull()),
            sqlite::Column("file_path", &AssetFileModel::filePath, sqlite::Unique(), sqlite::NotNull()),
            sqlite::Column("file_path_depth", &AssetFileModel::filePathDepth, sqlite::NotNull()),
            sqlite::Column("file_write_time", &AssetFileModel::fileWriteTime),
            sqlite::Column("file_size", &AssetFileModel::fileSize),
            sqlite::Column("source_path", &AssetFileModel::sourcePath),
            sqlite::Column("source_write_time", &AssetFileModel::sourceWriteTime),
            sqlite::Column("source_size", &AssetFileModel::sourceSize),
            sqlite::Column("scan_token", &AssetFileModel::scanToken)),
        sqlite::Index("idx_asset_file_source_path", &AssetFileModel::sourcePath),
        sqlite::Table("asset",
            sqlite::Column("id", &AssetModel::id, sqlite::PrimaryKey()),
            sqlite::Column("uuid", &AssetModel::uuid, sqlite::Unique(), sqlite::NotNull()),
            sqlite::Column("asset_file_id", &AssetModel::assetFileId, sqlite::NotNull()),
            sqlite::Column("asset_type_name", &AssetModel::assetType, sqlite::NotNull()),
            sqlite::Column("name", &AssetModel::name, sqlite::NotNull()),
            sqlite::Column("state", &AssetModel::state, sqlite::NotNull()),
            sqlite::Column("data_hash", &AssetModel::dataHash),
            sqlite::Column("import_data_hash", &AssetModel::importDataHash),
            sqlite::Column("importer_id", &AssetModel::importerId),
            sqlite::Column("importer_version", &AssetModel::importerVersion),
            sqlite::Column("compiler_id", &AssetModel::compilerId),
            sqlite::Column("compiler_version", &AssetModel::compilerVersion),
            sqlite::ForeignKey(&AssetModel::assetFileId).References(&AssetFileModel::id).OnDeleteCascade()),
        sqlite::Index("idx_asset_fk_asset_file_id", &AssetModel::assetFileId),
        sqlite::Index("idx_asset_state", &AssetModel::state),
        sqlite::Table("config",
            sqlite::Column("id", &ConfigModel::id, sqlite::PrimaryKey()),
            sqlite::Column("key", &ConfigModel::key, sqlite::Unique(), sqlite::NotNull()),
            sqlite::Column("value", &ConfigModel::value)),
        sqlite::Table("tag",
            sqlite::Column("id", &TagModel::id, sqlite::PrimaryKey()),
            sqlite::Column("name", &TagModel::name, sqlite::Unique(), sqlite::NotNull())),
        sqlite::Table("asset_tag",
            sqlite::Column("id", &AssetTagModel::id, sqlite::PrimaryKey()),
            sqlite::Column("asset_id", &AssetTagModel::assetId, sqlite::NotNull()),
            sqlite::Column("tag_id", &AssetTagModel::tagId, sqlite::NotNull()),
            sqlite::ForeignKey(&AssetTagModel::assetId).References(&AssetModel::id),
            sqlite::ForeignKey(&AssetTagModel::tagId).References(&TagModel::id)),
        sqlite::Index("idx_asset_tag_fk_asset_id", &AssetTagModel::assetId),
        sqlite::Index("idx_asset_tag_fk_tag_id", &AssetTagModel::tagId),
        sqlite::Table("asset_reference",
            sqlite::Column("id", &AssetReferenceModel::id, sqlite::PrimaryKey()),
            sqlite::Column("from_asset_id", &AssetReferenceModel::fromAssetId),
            sqlite::Column("to_asset_id", &AssetReferenceModel::toAssetId),
            sqlite::ForeignKey(&AssetReferenceModel::fromAssetId).References(&AssetModel::id),
            sqlite::ForeignKey(&AssetReferenceModel::toAssetId).References(&AssetModel::id)),
        sqlite::Index("idx_asset_reference_fk_from_asset_id", &AssetReferenceModel::fromAssetId),
        sqlite::Index("idx_asset_reference_fk_to_asset_id", &AssetReferenceModel::toAssetId),
        sqlite::RawSql(R"(
            -- Full-Text searching of the asset table.
            CREATE VIRTUAL TABLE IF NOT EXISTS fts_asset USING fts5(name, content='asset', content_rowid='id');

            CREATE TRIGGER IF NOT EXISTS asset_ai AFTER INSERT ON asset BEGIN
                INSERT INTO fts_asset(rowid, name) VALUES (new.id, new.name);
            END;

            CREATE TRIGGER IF NOT EXISTS asset_ad AFTER DELETE ON asset BEGIN
                INSERT INTO fts_asset(fts_asset, rowid, name) VALUES ('delete', old.id, old.name);
            END;

            CREATE TRIGGER IF NOT EXISTS asset_au AFTER UPDATE ON asset BEGIN
                INSERT INTO fts_asset(fts_asset, rowid, name) VALUES ('delete', old.id, old.name);
                INSERT INTO fts_asset(rowid, name) VALUES (new.id, new.name);
            END;

            -- Full-Text searching of the tag table.
            CREATE VIRTUAL TABLE IF NOT EXISTS fts_tag USING fts5(name, content='tag', content_rowid='id');

            CREATE TRIGGER IF NOT EXISTS tag_ai AFTER INSERT ON tag BEGIN
                INSERT INTO fts_tag(rowid, name) VALUES (new.id, new.name);
            END;

            CREATE TRIGGER IF NOT EXISTS tag_ad AFTER DELETE ON tag BEGIN
                INSERT INTO fts_tag(fts_tag, rowid, name) VALUES ('delete', old.id, old.name);
            END;

            CREATE TRIGGER IF NOT EXISTS tag_au AFTER UPDATE ON tag BEGIN
                INSERT INTO fts_tag(fts_tag, rowid, name) VALUES ('delete', old.id, old.name);
                INSERT INTO fts_tag(rowid, name) VALUES (new.id, new.name);
            END;
        )"));

    using AssetDbStorage = sqlite::Storage<Decay<decltype(AssetDbSchema)>>;
}

namespace he::sqlite
{
    template <typename T> struct SqlDataTypeTraits;

    template <typename T>
    struct SqlDataTypeTraits<assets::_UuidWrapper<T>>
    {
        static constexpr StringView SqlType = "BLOB(16)";
        static bool Bind(Statement& stmt, int32_t index, const assets::_UuidWrapper<T>& value) { return stmt.Bind(index, value.val.m_bytes); }
        static void Read(const ColumnReader& column, assets::_UuidWrapper<T>& value) { column.ReadBlob(value.val.m_bytes); }
        static void Write(StringBuilder& sql, const assets::_UuidWrapper<T>& value) { sql.Write("X'{:02x}'", FmtJoin(value.val.m_bytes, value.val.m_bytes + sizeof(value.val.m_bytes), "")); }
    };

    template <>
    struct SqlDataTypeTraits<Name>
    {
        static constexpr StringView SqlType = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const Name& value) { return stmt.Bind(index, value.String()); }
        static void Read(const ColumnReader& column, Name& value) { value = column.AsText(); }
        static void Write(StringBuilder& sql, const Name& value) { sql.Write("'{}'", value.String()); }
    };

}
