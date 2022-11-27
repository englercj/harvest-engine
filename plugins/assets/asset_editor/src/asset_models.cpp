// Copyright Chad Engler

#include "he/assets/asset_models.h"

#include "he/assets/asset_database.h"
#include "he/assets/types_fmt.h"
#include "he/core/hash.h"
#include "he/core/uuid_fmt.h"
#include "he/sqlite/database.h"
#include "he/sqlite/statement.h"

#include <unordered_set>

namespace he::assets
{
    static bool Bind(const sqlite::Statement& stmt, const AssetFileModel& model)
    {
        if (!stmt.Bind(1, model.uuid.val.m_bytes))
            return false;

        if (!stmt.Bind(2, model.file.path))
            return false;

        if (!stmt.Bind(3, BitCast<int64_t>(model.file.writeTime.val)))
            return false;

        if (!stmt.Bind(4, model.file.size))
            return false;

        if (!stmt.Bind(5, model.source.path))
            return false;

        if (!stmt.Bind(6, BitCast<int64_t>(model.source.writeTime.val)))
            return false;

        if (!stmt.Bind(7, model.source.size))
            return false;

        return true;
    }

    static void Read(const sqlite::Statement& stmt, AssetFileModel& model)
    {
        stmt.GetColumn(0).ReadBlob(model.uuid.val.m_bytes);
        model.file.path = stmt.GetColumn(1).GetText();
        model.file.writeTime.val = BitCast<uint64_t>(stmt.GetColumn(2).GetInt64());
        model.file.size = stmt.GetColumn(3).GetUint();
        model.source.path = stmt.GetColumn(4).GetText();
        model.source.writeTime.val = BitCast<uint64_t>(stmt.GetColumn(5).GetInt64());
        model.source.size = stmt.GetColumn(6).GetUint();
    }

    static bool Bind(const sqlite::Statement& stmt, const AssetModel& model, int64_t assetFileId)
    {
        if (!stmt.Bind(1, model.uuid.val.m_bytes))
            return false;

        if (!stmt.Bind(2, assetFileId))
            return false;

        if (!stmt.Bind(3, model.type))
            return false;

        if (!stmt.Bind(4, model.name))
            return false;

        if (!stmt.Bind(5, static_cast<uint32_t>(model.state)))
            return false;

        if (!stmt.Bind(6, model.dataHash))
            return false;

        if (!stmt.Bind(7, model.importDataHash))
            return false;

        if (!stmt.Bind(8, model.importerId))
            return false;

        if (!stmt.Bind(9, model.importerVersion))
            return false;

        if (!stmt.Bind(10, model.compilerId))
            return false;

        if (!stmt.Bind(11, model.compilerVersion))
            return false;

        return true;
    }

    static void Read(const sqlite::Statement& stmt, AssetModel& model)
    {
        stmt.GetColumn(0).ReadBlob(model.uuid.val.m_bytes);
        stmt.GetColumn(1).ReadBlob(model.fileUuid.val.m_bytes);
        model.type = stmt.GetColumn(2).GetText();
        model.name = stmt.GetColumn(3).GetText();
        model.state = AssetState(stmt.GetColumn(4).GetUint());
        model.dataHash = stmt.GetColumn(5).GetUint();
        model.importDataHash = stmt.GetColumn(6).GetUint();
        model.importerId = stmt.GetColumn(7).GetUint();
        model.importerVersion = stmt.GetColumn(8).GetUint();
        model.compilerId = stmt.GetColumn(9).GetUint();
        model.compilerVersion = stmt.GetColumn(10).GetUint();
    }

    bool AssetFileModel::AddOrUpdate(AssetDatabase& db, schema::AssetFile::Reader file, const AssetFileModel& model)
    {
        HE_ASSERT(model.uuid == file.GetUuid());

        sqlite::Transaction transaction = db.BeginTransaction();

        AssetFileUuid ids[2];
        String paths[2];
        uint32_t count = 0;

        // Read existing items to see what kind of operation we need to perform.
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT uuid, path FROM asset_file WHERE uuid = ? OR path = ?
            )");

            if (!stmt->Bind(1, model.uuid.val.m_bytes))
                return false;

            if (!stmt->Bind(2, model.file.path))
                return false;

            const bool res = stmt->EachRow([&](const sqlite::Statement& stmt)
            {
                // `uuid` and `path` columns are both unique columns, so the query above should never be
                // able to return more than 2 rows.
                HE_ASSERT(count < 2,
                    HE_MSG("Impossible row count from asset_file query. The DB schema shouldn't allow more than 2 results."),
                    HE_KV(count, count));

                stmt.GetColumn(0).ReadBlob(ids[count].val.m_bytes);
                paths[count] = stmt.GetColumn(1).GetText();
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

            if (paths[0] == model.file.path)
                i = 0;
            else if (count > 1 && paths[1] == model.file.path)
                i = 1;

            if (i != static_cast<uint32_t>(-1))
            {
                if (!AssetFileModel::RemoveOne(db, ids[i]))
                    return false;
            }
        }

        // Insert or update the file
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                INSERT INTO asset_file
                    (uuid, file_path, file_write_time, file_size, source_path, source_write_time, source_size)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT (uuid) DO UPDATE SET
                    file_path = excluded.file_path,
                    file_write_time = excluded.file_write_time,
                    file_size = excluded.file_size,
                    source_path = excluded.source_path,
                    source_write_time = excluded.source_write_time,
                    source_size = excluded.source_size
            )");

            if (!Bind(*stmt, model))
                return false;

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Get the id of the file row that was just inserted or updates
        int64_t assetFileId = 0;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset_file WHERE uuid = ?
            )");

            if (!stmt->Bind(1, model.uuid.val.m_bytes))
                return false;

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Row, HE_MSG("Failed to find a row we just inserted.")))
                return false;

            assetFileId = stmt->GetColumn(0).GetInt64();

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Read existing Asset IDs so we can remove ones that no longer exist.
        std::unordered_set<AssetUuid> assetUuids;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT uuid FROM asset WHERE asset_file_id = ?
            )");

            if (!stmt->Bind(1, assetFileId))
                return false;

            const bool res = stmt->EachRow([&](const sqlite::Statement& stmt)
            {
                AssetUuid assetId;
                stmt.GetColumn(0).ReadBlob(assetId.val.m_bytes);
                assetUuids.insert(assetId);
            });

            if (!res)
                return false;
        }

        // Add or update each existing asset in the file
        for (const schema::Asset::Reader asset : file.GetAssets())
        {
            AssetModel::AddOrUpdate(db, model.uuid, asset);

            const AssetUuid assetId(asset.GetUuid());
            assetUuids.erase(assetId);
        }

        // Remove each asset that no longer exists in the file.
        for (const AssetUuid& uuid : assetUuids)
        {
            AssetModel::RemoveOne(db, uuid);
        }

        transaction.Commit();
        return true;
    }

    bool AssetFileModel::FindOne(AssetDatabase& db, const AssetFileUuid& fileUuid, AssetFileModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE uuid = ?
        )");

        if (!stmt->Bind(1, fileUuid.val.m_bytes))
            return false;

        if (stmt->Step() == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
        }

        return false;
    }

    bool AssetFileModel::FindOne(AssetDatabase& db, const char* path, AssetFileModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE path = ?
        )");

        if (!stmt->Bind(1, path))
            return false;

        if (stmt->Step() == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
        }

        return false;
    }

    bool AssetFileModel::RemoveOne(AssetDatabase& db, const AssetFileUuid& fileUuid)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELETE FROM asset_file WHERE uuid = ?
        )");

        if (!stmt->Bind(1, fileUuid.val.m_bytes))
            return false;

        return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
    }

    bool AssetFileModel::RemoveOne(AssetDatabase& db, const char* path)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELETE FROM asset_file WHERE file_path = ?
        )");

        if (!stmt->Bind(1, path))
            return false;

        return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
    }

    bool AssetFileModel::RemoveOutdated(AssetDatabase& db, uint32_t scanToken)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELETE FROM asset_file WHERE scan_token != ?
        )");

        if (!stmt->Bind(1, scanToken))
            return false;

        return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);

    }

    bool AssetFileModel::UpdateScanToken(AssetDatabase& db, const AssetFileUuid& fileUuid, uint32_t scanToken)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            UPDATE asset_file SET scan_token = ? WHERE uuid = ?
        )");

        if (!stmt->Bind(1, scanToken))
            return false;

        if (!stmt->Bind(2, fileUuid.val.m_bytes))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetFileModel::UpdateScanToken(AssetDatabase& db, const char* path, uint32_t scanToken)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            UPDATE asset_file SET scan_token = ? WHERE file_path = ?
        )");

        if (!stmt->Bind(1, scanToken))
            return false;

        if (!stmt->Bind(2, path))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::AddOrUpdate(AssetDatabase& db, const AssetModel& model)
    {
        sqlite::Transaction transaction = db.BeginTransaction();

        // Get the id of the file row this asset belongs to
        int64_t assetFileId = 0;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset_file WHERE uuid = ?
            )");

            if (!stmt->Bind(1, model.fileUuid.val.m_bytes))
                return false;

            if (stmt->Step() != sqlite::StepResult::Row)
            {
                HE_LOG_ERROR(he_sqlite,
                    HE_MSG("Failed to add or update asset. No file row found with uuid."),
                    HE_KV(asset_uuid, model.uuid),
                    HE_KV(asset_name, model.name),
                    HE_KV(asset_file_uuid, model.fileUuid));
                return false;
            }

            assetFileId = stmt->GetColumn(0).GetInt64();

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset
                (uuid, asset_file_id, type, name, state, import_data_hash, importer_id, importer_version, compiler_id, compiler_version)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT (id) DO UPDATE SET
                asset_file_id = excluded.asset_file_id,
                type = excluded.type,
                name = excluded.name,
                state = excluded.state,
                import_data_hash = excluded.import_data_hash,
                importer_id = excluded.importer_id,
                importer_version = excluded.importer_version,
                compiler_id = excluded.compiler_id,
                compiler_version = excluded.compiler_version
        )");

        if (!Bind(*stmt, model, assetFileId))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::AddOrUpdate(AssetDatabase& db, const AssetFileUuid& fileUuid, schema::Asset::Reader asset)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset
                (uuid, asset_file_id, type, name, state, import_data_hash)
            VALUES (?, ?, ?, ?, ?, ?)
            ON CONFLICT (id) DO UPDATE SET
                asset_file_id = excluded.asset_file_id,
                type = excluded.type,
                name = excluded.name,
                import_data_hash = excluded.import_data_hash
        )");

        if (!stmt->Bind(1, asset.GetUuid().GetValue()))
            return false;

        if (!stmt->Bind(2, fileUuid.val.m_bytes))
            return false;

        if (!stmt->Bind(3, asset.GetType()))
            return false;

        if (!stmt->Bind(4, asset.GetName()))
            return false;

        if (!stmt->Bind(5, static_cast<uint16_t>(AssetState::Unknown)))
            return false;

        he::schema::AnyStruct::Reader ptr = asset.GetImportData();
        const uint32_t importDataHash = ptr.IsNull() ? 0 : FNV32::HashData(ptr.Target(), ptr.StructWordSize());
        if (!stmt->Bind(6, importDataHash))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::FindOne(AssetDatabase& db, const AssetUuid& assetUuid, AssetModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset WHERE uuid = ?
        )");

        if (!stmt->Bind(1, assetUuid.val.m_bytes))
            return false;

        const sqlite::StepResult res = stmt->Step();

        if (res == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return stmt->Step() == sqlite::StepResult::Done;
        }

        return false;
    }

    bool AssetModel::FindAll(AssetDatabase& db, const AssetFileUuid& fileUuid, Vector<AssetModel>& models)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset WHERE asset_file_id = ?
        )");

        if (!stmt->Bind(1, fileUuid.val.m_bytes))
            return false;

        return stmt->EachRow([&](const sqlite::Statement& stmt)
        {
            Read(stmt, models.EmplaceBack());
        });
    }

    bool AssetModel::FindAll(AssetDatabase& db, const char* search, Vector<AssetModel>& models)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM fts_asset WHERE fts_asset MATCH ? ORDER BY rank
        )");

        if (!stmt->Bind(1, search))
            return false;

        return stmt->EachRow([&](const sqlite::Statement& stmt)
        {
            Read(stmt, models.EmplaceBack());
        });
    }

    bool AssetModel::FindAll(AssetDatabase& db, AssetState state, Vector<AssetModel>& models)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset WHERE state = ?
        )");

        if (!stmt->Bind(1, static_cast<uint8_t>(state)))
            return false;

        return stmt->EachRow([&](const sqlite::Statement& stmt)
        {
            Read(stmt, models.EmplaceBack());
        });
    }

    bool AssetModel::RemoveOne(AssetDatabase& db, const AssetUuid& assetUuid)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELTE FROM asset WHERE uuid = ?
        )");

        if (!stmt->Bind(1, assetUuid.val.m_bytes))
            return false;

        return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
    }

    bool AssetModel::AddTag(AssetDatabase& db, const AssetUuid& assetUuid, const char* tag)
    {
        sqlite::Transaction transaction = db.BeginTransaction();

        // Get the id of the asset entry
        int64_t assetId = 0;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset WHERE uuid = ?
            )");

            if (!stmt->Bind(1, assetUuid.val.m_bytes))
                return false;

            if (stmt->Step() != sqlite::StepResult::Row)
                return false;

            assetId = stmt->GetColumn(0).GetInt64();

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Ensure the tag is in the table
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                INSERT INTO tag (name) VALUES (?)
                ON CONFLICT (name) DO NOTHING
            )");

            if (!stmt->Bind(1, tag))
                return false;

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Get the id of the tag entry
        int64_t tagId = 0;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM tag WHERE name = ?
            )");

            if (!stmt->Bind(1, tag))
                return false;

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Row, HE_MSG("Failed to find a row we just inserted.")))
                return false;

            tagId = stmt->GetColumn(0).GetInt64();

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Link the asset to the tag
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                INSERT INTO asset_tag (asset_id, tag_id) VALUES (?, ?)
            )");

            if (!stmt->Bind(1, assetId))
                return false;

            if (!stmt->Bind(2, tagId))
                return false;

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        transaction.Commit();
        return true;
    }

    bool AssetModel::RemoveTag(AssetDatabase& db, const AssetUuid& assetUuid, const char* tag)
    {
        sqlite::Transaction transaction = db.BeginTransaction();

        // Get the id of the asset entry
        int64_t assetId = 0;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset WHERE uuid = ?
            )");

            if (!stmt->Bind(1, assetUuid.val.m_bytes))
                return false;

            if (stmt->Step() != sqlite::StepResult::Row)
                return false;

            assetId = stmt->GetColumn(0).GetInt64();

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Get the id of the tag entry
        int64_t tagId = 0;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM tag WHERE name = ?
            )");

            if (!stmt->Bind(1, tag))
                return false;

            if (stmt->Step() != sqlite::StepResult::Row)
                return false;

            tagId = stmt->GetColumn(0).GetInt64();

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Remove the tag
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                DELETE FROM tag WHERE asset_id = ? AND tag_id = ?
            )");

            if (!stmt->Bind(1, assetId))
                return false;

            if (!stmt->Bind(2, tagId))
                return false;

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        transaction.Commit();
        return true;
    }

    bool AssetModel::RemoveAllTags(AssetDatabase& db, const AssetUuid& assetUuid)
    {
        sqlite::Transaction transaction = db.BeginTransaction();

        // Get the id of the asset row
        int64_t assetId = 0;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset WHERE uuid = ?
            )");

            if (!stmt->Bind(1, assetUuid.val.m_bytes))
                return false;

            if (stmt->Step() != sqlite::StepResult::Row)
                return false;

            assetId = stmt->GetColumn(0).GetInt64();

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        // Remove all the tags for the asset
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                DELETE FROM tag WHERE asset_id = ?
            )");

            if (!stmt->Bind(1, assetId))
                return false;

            if (!HE_VERIFY(stmt->Step() == sqlite::StepResult::Done))
                return false;
        }

        transaction.Commit();
        return true;
    }

    bool AssetModel::UpdateState(AssetDatabase& db, const AssetUuid& assetUuid, AssetState state)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            UPDATE asset SET state = ? WHERE uuid = ?
        )");

        if (!stmt->Bind(1, static_cast<uint8_t>(state)))
            return false;

        if (!stmt->Bind(2, assetUuid.val.m_bytes))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool ConfigModel::AddOrUpdate(AssetDatabase& db, const ConfigModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO config (key, value)
            VALUES (?, ?)
            ON CONFLICT (key) DO UPDATE SET value = excluded.value
        )");

        if (!stmt->Bind(1, model.key))
            return false;

        if (!stmt->Bind(2, model.value))
            return false;

        return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
    }

    bool ConfigModel::FindOne(AssetDatabase& db, const char* key, ConfigModel& outModel)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM config WHERE key = ?
        )");

        if (!stmt->Bind(1, key))
            return false;

        const sqlite::StepResult res = stmt->Step();

        if (res == sqlite::StepResult::Row)
        {
            outModel.key = stmt->GetColumn(0).GetText();
            outModel.value = stmt->GetColumn(1).GetBlob();
            return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
        }

        return false;
    }
}
