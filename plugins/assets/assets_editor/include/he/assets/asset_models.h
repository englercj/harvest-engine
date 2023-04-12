// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/string_builder.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/core/uuid.h"
#include "he/sqlite/orm.h"

namespace he::assets
{
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
        uint32_t id{ 0 };
        AssetFileUuid uuid{};

        String filePath{};
        uint32_t filePathDepth{ 0 };
        SystemTime fileWriteTime{ 0 };
        uint32_t fileSize{ 0 };

        String sourcePath{};
        SystemTime sourceWriteTime{ 0 };
        uint32_t sourceSize{ 0 };

        uint32_t scanToken{ 0 };
    };

    struct AssetModel final
    {
        uint32_t id{ 0 };
        AssetUuid uuid{};
        uint32_t assetFileId{ 0 };
        String assetTypeName{}; // TODO: Change this to store only the hash value. We can create a join table that maps IDs -> string names for debugging.
        String name{};
        AssetState state{ AssetState::Unknown };
        uint32_t dataHash{ 0 };
        uint32_t importDataHash{ 0 };
        uint32_t importerId{ 0 };
        uint32_t importerVersion{ 0 };
        uint32_t compilerId{ 0 };
        uint32_t compilerVersion{ 0 };
    };

    struct ConfigModel final
    {
        uint32_t id{ 0 };
        String key{};
        Vector<uint8_t> value{};
    };

    struct TagModel final
    {
        uint32_t id{ 0 };
        String name{};
    };

    struct AssetTagModel final
    {
        uint32_t id{ 0 };
        uint32_t assetId{ 0 };
        uint32_t tagId{ 0 };
    };

    struct AssetReferenceModel final
    {
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
            sqlite::Column("uuid", &AssetFileModel::uuid, sqlite::Unique()),
            sqlite::Column("file_path", &AssetFileModel::filePath, sqlite::Unique()),
            sqlite::Column("file_path_depth", &AssetFileModel::filePathDepth),
            sqlite::Column("file_write_time", &AssetFileModel::fileWriteTime),
            sqlite::Column("file_size", &AssetFileModel::fileSize),
            sqlite::Column("source_path", &AssetFileModel::sourcePath),
            sqlite::Column("source_write_time", &AssetFileModel::sourceWriteTime),
            sqlite::Column("source_size", &AssetFileModel::sourceSize),
            sqlite::Column("scan_token", &AssetFileModel::scanToken)),
        sqlite::Table("asset",
            sqlite::Column("id", &AssetModel::id, sqlite::PrimaryKey()),
            sqlite::Column("uuid", &AssetModel::uuid, sqlite::Unique()),
            sqlite::Column("asset_file_id", &AssetModel::assetFileId),
            sqlite::Column("asset_type_name", &AssetModel::assetTypeName),
            sqlite::Column("name", &AssetModel::name),
            sqlite::Column("state", &AssetModel::state),
            sqlite::Column("data_hash", &AssetModel::dataHash),
            sqlite::Column("import_data_hash", &AssetModel::importDataHash),
            sqlite::Column("importer_id", &AssetModel::importerId),
            sqlite::Column("importer_version", &AssetModel::importerVersion),
            sqlite::Column("compiler_id", &AssetModel::compilerId),
            sqlite::Column("compiler_version", &AssetModel::compilerVersion),
            sqlite::ForeignKey(&AssetModel::assetFileId).References(&AssetFileModel::id)),
        sqlite::Table("config",
            sqlite::Column("id", &ConfigModel::id, sqlite::PrimaryKey()),
            sqlite::Column("key", &ConfigModel::key, sqlite::Unique()),
            sqlite::Column("value", &ConfigModel::value)),
        sqlite::Table("tag",
            sqlite::Column("id", &TagModel::id, sqlite::PrimaryKey()),
            sqlite::Column("name", &TagModel::name, sqlite::Unique())),
        sqlite::Table("asset_tag",
            sqlite::Column("id", &AssetTagModel::id, sqlite::PrimaryKey()),
            sqlite::Column("asset_id", &AssetTagModel::assetId),
            sqlite::Column("tag_id", &AssetTagModel::tagId),
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
            CREATE VIRTUAL TABLE fts_asset USING fts5(name, content='asset', content_rowid='id');

            CREATE TRIGGER asset_ai AFTER INSERT ON asset BEGIN
                INSERT INTO fts_asset(rowid, name) VALUES (new.id, new.name);
            END;

            CREATE TRIGGER asset_ad AFTER DELETE ON asset BEGIN
                INSERT INTO fts_asset(fts_asset, rowid, name) VALUES ('delete', old.id, old.name);
            END;

            CREATE TRIGGER asset_au AFTER UPDATE ON asset BEGIN
                INSERT INTO fts_asset(fts_asset, rowid, name) VALUES ('delete', old.id, old.name);
                INSERT INTO fts_asset(rowid, name) VALUES (new.id, new.name);
            END;

            -- Full-Text searching of the tag table.
            CREATE VIRTUAL TABLE fts_tag USING fts5(name, content='tag', content_rowid='id');

            CREATE TRIGGER tag_ai AFTER INSERT ON tag BEGIN
                INSERT INTO fts_tag(rowid, name) VALUES (new.id, new.name);
            END;

            CREATE TRIGGER tag_ad AFTER DELETE ON tag BEGIN
                INSERT INTO fts_tag(fts_tag, rowid, name) VALUES ('delete', old.id, old.name);
            END;

            CREATE TRIGGER tag_au AFTER UPDATE ON tag BEGIN
                INSERT INTO fts_tag(fts_tag, rowid, name) VALUES ('delete', old.id, old.name);
                INSERT INTO fts_tag(rowid, name) VALUES (new.id, new.name);
            END;
        )"));
}

namespace he::sqlite
{
    template <typename T> struct SqlDataTypeTraits;

    // AssetFileUuid
    template <>
    struct sqlite::SqlDataTypeTraits<assets::AssetFileUuid>
    {
        static constexpr StringView Sql = "BLOB(16)";
        static bool Bind(Statement& stmt, int32_t index, const assets::AssetFileUuid& value) { stmt.Bind(index, value.val.m_bytes); }
        static void Read(const ColumnReader& column, assets::AssetFileUuid& value) { column.ReadBlob(value.val.m_bytes); }
        static void Write(StringBuilder& sql, const assets::AssetFileUuid& value) { sql.Write("X'{:02x}'", FmtJoin(value.val.m_bytes, value.val.m_bytes + sizeof(value.val.m_bytes), "")); }
    };
}
