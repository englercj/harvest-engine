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

    bool AddOrUpdateAssetFile(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model)
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

            // TODO: Cols() doesn't support
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
