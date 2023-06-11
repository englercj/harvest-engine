// Copyright Chad Engler

#include "he/assets/asset_models.h"

#include "he/assets/asset_database.h"
#include "he/assets/types_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/hash.h"
#include "he/core/hash_table.h"
#include "he/core/uuid_fmt.h"
#include "he/schema/layout.h"
#include "he/sqlite/database.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/statement.h"

using namespace he::sqlite;

namespace he::assets
{
    static uint32_t GetPathDepth(StringView path)
    {
        uint32_t depth = 0;
        const char* p = String::FindN(path.Data(), path.Size(), '/');
        while (p)
        {
            ++depth;
            ++p;

            if (p >= path.End())
                break;

            p = String::FindN(p, static_cast<uint32_t>(path.End() - p), '/');
        }

        return depth;
    }

    static int64_t GetAssetRowId(AssetDatabase& db, const AssetUuid& assetUuid)
    {
        const auto query = Select<AssetModel>(Cols(&AssetModel::id), Where(Col(&AssetModel::uuid) == assetUuid));

        int64_t assetId = 0;
        db.Storage().Execute(query, [&](const Statement& stmt)
        {
            ReadSql(stmt.GetColumn(0), assetId);
        });

        return assetId;
    }

    static int64_t GetTagRowId(AssetDatabase& db, StringView tag)
    {
        const auto query = Select<TagModel>(Cols(&TagModel::id), Where(Col(&TagModel::name) == tag));

        int64_t tagId = 0;
        db.Storage().Execute(query, [&](const Statement& stmt)
        {
            ReadSql(stmt.GetColumn(0), tagId);
        });

        return tagId;
    }

    bool AssetFileModel::AddOrUpdate(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model)
    {
        HE_ASSERT(model.uuid == file.GetUuid());

        sqlite::Transaction transaction(db.Handle());

        AssetFileUuid ids[2];
        String paths[2];
        uint32_t count = 0;

        // Read existing items to see what kind of operation we need to perform.
        {
            const auto query = Select<AssetFileModel>(
                Cols(&AssetFileModel::uuid, &AssetFileModel::filePath),
                Where(Col(&AssetFileModel::uuid) == model.uuid || Col(&AssetFileModel::filePath) == model.filePath));

            const bool res = db.Storage().Execute(query, [&](const sqlite::Statement& stmt)
            {
                // `uuid` and `path` columns are both unique columns, so the query above should never be
                // able to return more than 2 rows.
                HE_ASSERT(count < 2,
                    HE_MSG("Impossible row count from asset_file query. The DB schema shouldn't allow more than 2 results."),
                    HE_KV(count, count));

                ReadSql(stmt.GetColumn(0), ids[count].val);
                ReadSql(stmt.GetColumn(1), paths[count]);
                ++count;
            });

            if (!res)
                return false;
        }

        // If we found any then a row may exists with the same path as the file we wish to
        // insert. If that is the case, we need to remove it first. An ID conflict is handled in
        // the upsert query later.
        // Because both `uuid` and `path` columns are unique we can't match *both* rows with path
        // or uuid. Since a uuid conflict is handled below in the query, we only need to fix a path
        // match ahead of time. Paths matching, but not uuid, is also the least likely to happen.
        if (count > 0)
        {
            uint32_t i = static_cast<uint32_t>(-1);

            if (paths[0] == model.filePath)
                i = 0;
            else if (count > 1 && paths[1] == model.filePath)
                i = 1;

            if (i != static_cast<uint32_t>(-1))
            {
                if (!db.Storage().Destroy<AssetFileModel>(Where(Col(&AssetFileModel::uuid) == ids[i])))
                    return false;
            }
        }

        // Insert or update the file
        {
            const auto query = Insert<AssetFileModel>(
                Cols(&AssetFileModel::uuid, &AssetFileModel::filePath, &AssetFileModel::filePathDepth, &AssetFileModel::fileWriteTime, &AssetFileModel::fileSize, &AssetFileModel::sourcePath, &AssetFileModel::sourceWriteTime, &AssetFileModel::sourceSize),
                Values(model.uuid, model.filePath, GetPathDepth(model.filePath), model.fileWriteTime, model.fileSize, model.sourcePath, model.sourceWriteTime, model.sourceSize),
                OnConflict(&AssetFileModel::uuid).DoUpdate(
                    Set(&AssetFileModel::filePath, Excluded(&AssetFileModel::filePath)),
                    Set(&AssetFileModel::filePathDepth, Excluded(&AssetFileModel::filePathDepth)),
                    Set(&AssetFileModel::fileWriteTime, Excluded(&AssetFileModel::fileWriteTime)),
                    Set(&AssetFileModel::fileSize, Excluded(&AssetFileModel::fileSize)),
                    Set(&AssetFileModel::sourcePath, Excluded(&AssetFileModel::sourcePath)),
                    Set(&AssetFileModel::sourceWriteTime, Excluded(&AssetFileModel::sourceWriteTime)),
                    Set(&AssetFileModel::sourceSize, Excluded(&AssetFileModel::sourceSize))));

            if (!db.Storage().Execute(query))
                return false;
        }

        // Get the id of the file row that was just inserted or updates
        int64_t assetFileId = 0;
        {
            const auto query = Select<AssetFileModel>(
                Cols(&AssetFileModel::id),
                Where(Col(&AssetFileModel::uuid) == model.uuid));

            const bool res = db.Storage().Execute(query, [&](const sqlite::Statement& stmt)
            {
                ReadSql(stmt.GetColumn(0), assetFileId);
            });

            if (!res)
                return false;
        }

        // Read existing Asset IDs so we can remove ones that no longer exist.
        HashSet<AssetUuid> assetUuids;
        {
            const auto query = Select<AssetModel>(
                Cols(&AssetModel::uuid),
                Where(Col(&AssetModel::assetFileId) == assetFileId));

            const bool res = db.Storage().Execute(query, [&](const sqlite::Statement& stmt)
            {
                AssetUuid assetId;
                ReadSql(stmt.GetColumn(0), assetId);
                assetUuids.Insert(assetId);
            });

            if (!res)
                return false;
        }

        // Add or update each existing asset in the file
        for (const Asset::Reader asset : file.GetAssets())
        {
            const uint32_t dataHash = CalculateHash<CRC32C>(asset.GetData());
            const uint32_t importDataHash = CalculateHash<CRC32C>(asset.GetData());

            const auto query = Insert<AssetModel>(
                Cols(&AssetModel::uuid, &AssetModel::assetFileId, &AssetModel::assetType, &AssetModel::name, &AssetModel::state, &AssetModel::dataHash, &AssetModel::importDataHash),
                Select<AssetFileModel>(
                    Cols(AssetUuid(asset.GetUuid()), &AssetFileModel::id, asset.GetType().AsView(), asset.GetName().AsView(), AssetState::Unknown, dataHash, importDataHash),
                    Where(Col(&AssetFileModel::uuid) == model.uuid)),
                OnConflict(&AssetModel::uuid).DoUpdate(
                    Set(&AssetModel::assetFileId, Excluded(&AssetModel::assetFileId)),
                    Set(&AssetModel::assetType, Excluded(&AssetModel::assetType)),
                    Set(&AssetModel::name, Excluded(&AssetModel::name)),
                    Set(&AssetModel::dataHash, Excluded(&AssetModel::dataHash)),
                    Set(&AssetModel::importDataHash, Excluded(&AssetModel::importDataHash))));

            if (!db.Storage().Execute(query))
                return false;

            const AssetUuid assetId(asset.GetUuid());
            assetUuids.Erase(assetId);
        }

        // Remove each asset that no longer exists in the file.
        for (const AssetUuid& uuid : assetUuids)
        {
            if (!db.Storage().Destroy<AssetModel>(Where(Col(&AssetModel::uuid) == uuid)))
                return false;
        }

        transaction.Commit();
        return true;
    }

    bool AssetModel::FindAllByAssetFilePath(AssetDatabase& db, Vector<AssetModel>& models, StringView pathPrefix)
    {
        // TODO: JOIN support in ORM so these queries don't need to be raw.

        Statement stmt;

        // Special case handling for the root. The LIKE search on path is omitted here because it
        // would always be true when searching the root path.
        if (pathPrefix.IsEmpty() || pathPrefix == "/")
        {
            constexpr const char SqlQueryRoot[] = R"(
                SELECT * FROM asset
                JOIN asset_file ON asset_file.id = asset.asset_file_id
                WHERE asset_file.file_path_depth = ?
            )";

            if (!stmt.Prepare(db.Handle(), SqlQueryRoot))
                return false;

            if (!stmt.Bind(1, 0))
                return false;
        }
        else
        {
            constexpr char SqlQueryPathWithSlash[] = R"(
                SELECT * FROM asset
                JOIN asset_file ON asset_file.id = asset.asset_file_id
                WHERE asset_file.file_path_depth = ? AND asset_file.file_path LIKE ? || '%'
            )";

            constexpr char SqlQueryPathWithoutSlash[] = R"(
                SELECT * FROM asset
                JOIN asset_file ON asset_file.id = asset.asset_file_id
                WHERE asset_file.file_path_depth = ? AND asset_file.file_path LIKE ? || '/%'
            )";

            const bool hasSlash = pathPrefix.Back() == '/';
            const char* query = hasSlash ? SqlQueryPathWithSlash : SqlQueryPathWithoutSlash;

            if (!stmt.Prepare(db.Handle(), query))
                return false;

            if (!stmt.Bind(1, GetPathDepth(pathPrefix)))
                return false;

            if (!stmt.Bind(2, pathPrefix))
                return false;
        }

        return stmt.EachRow([&](const sqlite::Statement& stmt)
        {
            AssetModel& model = models.EmplaceBack();

            ReadSql(stmt.GetColumn(0), model.id);
            ReadSql(stmt.GetColumn(1), model.uuid);
            ReadSql(stmt.GetColumn(2), model.assetFileId);
            ReadSql(stmt.GetColumn(3), model.assetType);
            ReadSql(stmt.GetColumn(4), model.name);
            ReadSql(stmt.GetColumn(5), model.state);
            ReadSql(stmt.GetColumn(6), model.dataHash);
            ReadSql(stmt.GetColumn(7), model.importDataHash);
            ReadSql(stmt.GetColumn(8), model.importerId);
            ReadSql(stmt.GetColumn(9), model.importerVersion);
            ReadSql(stmt.GetColumn(10), model.compilerId);
            ReadSql(stmt.GetColumn(11), model.compilerVersion);
        });
    }

    bool AssetTagModel::Add(AssetDatabase& db, const AssetUuid& assetUuid, StringView tag)
    {
        Transaction transaction(db.Handle());

        int64_t assetId = GetAssetRowId(db, assetUuid);
        if (assetId == 0)
            return false;

        const auto tagInsert = Insert<TagModel>(Cols(&TagModel::name), Values(tag), OnConflict(&TagModel::name).DoNothing());

        if (!db.Storage().Execute(tagInsert))
            return false;

        // We have to query this value instead of using `sqlite3_last_insert_rowid()` or a
        // RETURNING clause on the insert because those won't give us the right value if we
        // hit the conflict clause on the insertion above.
        const int64_t tagId = GetTagRowId(db, tag);
        if (tagId == 0)
            return false;

        const auto linkInsert = Insert<AssetTagModel>(
            Cols(&AssetTagModel::assetId, &AssetTagModel::tagId),
            Values(assetId, tagId));

        if (!db.Storage().Execute(linkInsert))
            return false;

        transaction.Commit();
        return true;
    }

    bool AssetTagModel::Remove(AssetDatabase& db, const AssetUuid& assetUuid, StringView tag)
    {
        Transaction transaction(db.Handle());

        const int64_t assetId = GetAssetRowId(db, assetUuid);
        if (assetId == 0)
            return false;

        const int64_t tagId = GetTagRowId(db, tag);
        if (tagId == 0)
            return false;

        auto constraint = Where(Col(&AssetTagModel::assetId) == assetId && Col(&AssetTagModel::tagId) == tagId);

        if (!db.Storage().Destroy<AssetTagModel>(Move(constraint)))
            return false;

        transaction.Commit();
        return true;
    }

    bool AssetTagModel::RemoveAll(AssetDatabase& db, const AssetUuid& assetUuid)
    {
        Transaction transaction(db.Handle());

        int64_t assetId = GetAssetRowId(db, assetUuid);
        if (assetId == 0)
            return false;

        auto constraint = Where(Col(&AssetTagModel::assetId) == assetId);

        if (!db.Storage().Destroy<AssetTagModel>(Move(constraint)))
            return false;

        transaction.Commit();
        return true;
    }
}

namespace he
{
    template <>
    const char* AsString(assets::AssetState x)
    {
        switch (x)
        {
            case assets::AssetState::Unknown: return "Unknown";
            case assets::AssetState::NeedsImport: return "NeedsImport";
            case assets::AssetState::NeedsCompile: return "NeedsCompile";
            case assets::AssetState::Ready: return "Ready";
            case assets::AssetState::ImportFailed: return "ImportFailed";
            case assets::AssetState::CompileFailed: return "CompileFailed";
        }

        return "<unknown>";
    }
}
