// Copyright Chad Engler

#include "he/assets/asset_models.h"

#include "he/assets/asset_database.h"
#include "he/core/hash.h"
#include "he/sqlite/database.h"
#include "he/sqlite/statement.h"

#include <unordered_set>

namespace he::assets
{
    static bool Bind(const sqlite::Statement& stmt, const AssetFileModel& model)
    {
        if (!stmt.Bind(1, model.id.m_bytes))
            return false;

        if (!stmt.Bind(2, model.path))
            return false;

        if (!stmt.Bind(3, BitCast<int64_t>(model.writeTime.val)))
            return false;

        if (!stmt.Bind(4, model.fileSize))
            return false;

        if (!stmt.Bind(5, model.source.path))
            return false;

        if (!stmt.Bind(6, BitCast<int64_t>(model.source.writeTime.val)))
            return false;

        if (!stmt.Bind(7, model.source.fileSize))
            return false;

        if (!stmt.Bind(8, model.scanToken))
            return false;

        return true;
    }

    static void Read(const sqlite::Statement& stmt, AssetFileModel& model)
    {
        stmt.GetColumn(0).ReadBlob(model.id.m_bytes);
        model.path = stmt.GetColumn(1).GetText();
        model.writeTime.val = BitCast<uint64_t>(stmt.GetColumn(2).GetInt64());
        model.fileSize = stmt.GetColumn(3).GetUint();
        model.source.path = stmt.GetColumn(4).GetText();
        model.source.writeTime.val = BitCast<uint64_t>(stmt.GetColumn(5).GetInt64());
        model.source.fileSize = stmt.GetColumn(6).GetUint();
        model.scanToken = stmt.GetColumn(7).GetUint();
    }

    static bool Bind(const sqlite::Statement& stmt, const AssetModel& model)
    {
        if (!stmt.Bind(1, model.id.m_bytes))
            return false;

        if (!stmt.Bind(2, model.fileId.m_bytes))
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
        stmt.GetColumn(0).ReadBlob(model.id.m_bytes);
        stmt.GetColumn(1).ReadBlob(model.fileId.m_bytes);
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

    bool AssetFileModel::AddOrUpdate(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model)
    {
        HE_ASSERT(ToUuid(file.GetId()) == model.id);

        sqlite::Transaction transaction = db.BeginTransaction();

        Uuid ids[2];
        size_t count = 0;

        // Read existing items to see what kind of operation we need to perform.
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset_file WHERE id = ? OR path = ?
            )");

            if (!stmt->Bind(1, model.id.m_bytes) || !stmt->Bind(2, model.path))
                return false;

            const bool res = stmt->EachRow([&](const sqlite::Statement& stmt)
            {
                stmt.GetColumn(0).ReadBlob(ids[count++].m_bytes);
            });

            if (!res)
                return false;
        }
        HE_ASSERT(count <= 2,
            HE_MSG("Impossible row count from asset_file query. Expected 2 or more."),
            HE_KV(count, count));

        // If we found any then a row may exists with the same path as the file we wish to
        // insert. If that is the case, we need to remove it first. An ID conflict is handled in
        // the upsert query later.
        if (count > 0)
        {
            size_t i = static_cast<size_t>(-1);

            if (ids[0] == model.id)
                i = 0;
            else if (count > 1 && ids[1] == model.id)
                i = 1;

            if (i != static_cast<size_t>(-1))
            {
                if (!AssetFileModel::RemoveOne(db, ids[i]))
                    return false;
            }
        }

        // Insert or update the file
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                INSERT INTO asset_file
                    (id, path, write_time, file_size, source_path, source_write_time, source_file_size, scan_token)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT (id) DO UPDATE SET
                    path = excluded.path,
                    write_time = excluded.last_write_time,
                    file_size = excluded.last_file_size,
                    source_path = excluded.source_path,
                    source_write_time = excluded.source_write_time,
                    source_file_size = excluded.source_file_size,
                    scan_token = excluded.last_session_token
            )");

            if (!Bind(*stmt, model))
                return false;

            if (stmt->Step() != sqlite::StepResult::Done)
                return false;
        }

        // Read existing Asset IDs so we can remove ones that no longer exist.
        std::unordered_set<Uuid> assetIds;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset WHERE asset_file_id = ?
            )");

            if (!stmt->Bind(1, model.id.m_bytes))
                return false;

            const bool res = stmt->EachRow([&](const sqlite::Statement& stmt)
            {
                Uuid assetId;
                stmt.GetColumn(0).ReadBlob(assetId.m_bytes);
                assetIds.insert(assetId);
            });

            if (!res)
                return false;
        }

        // Add or update each existing asset in the file
        for (const Asset::Reader asset : file.GetAssets())
        {
            AssetModel::AddOrUpdate(db, model.id, asset);

            const Uuid assetId = ToUuid(asset.GetId());
            assetIds.erase(assetId);
        }

        // Remove each asset that no longer exists in the file.
        for (const Uuid& id : assetIds)
        {
            AssetModel::RemoveOne(db, id);
        }

        transaction.Commit();
        return true;
    }

    bool AssetFileModel::FindOne(AssetDatabase& db, const Uuid& fileId, AssetFileModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE id = ?
        )");

        if (!stmt->Bind(1, fileId.m_bytes))
            return false;

        if (stmt->Step() == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return true;
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
            return true;
        }

        return false;
    }

    bool AssetFileModel::RemoveOne(AssetDatabase& db, const Uuid& fileId)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELETE FROM asset_file WHERE id = ?
        )");

        if (!stmt->Bind(1, fileId.m_bytes))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetFileModel::RemoveAll(AssetDatabase& db, uint32_t scanToken)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELETE FROM asset_file WHERE scan_token != ?
        )");

        if (!stmt->Bind(1, scanToken))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;

    }

    bool AssetFileModel::UpdateScanToken(AssetDatabase& db, const Uuid& fileId, uint32_t scanToken)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            UPDATE asset_file SET scan_token = ? WHERE id = ?
        )");

        if (!stmt->Bind(1, scanToken))
            return false;

        if (!stmt->Bind(2, fileId.m_bytes))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::AddOrUpdate(AssetDatabase& db, const AssetModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset
                (id, asset_file_id, type, name, state, import_data_hash, importer_id, importer_version, compiler_id, compiler_version)
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

        if (!Bind(*stmt, model))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::AddOrUpdate(AssetDatabase& db, const Uuid& fileId, Asset::Reader asset)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset
                (id, asset_file_id, type, name, state, import_data_hash)
            VALUES (?, ?, ?, ?, ?, ?)
            ON CONFLICT (id) DO UPDATE SET
                asset_file_id = excluded.asset_file_id,
                type = excluded.type,
                name = excluded.name,
                import_data_hash = excluded.import_data_hash
        )");

        if (!stmt->Bind(1, asset.GetId().GetValue()))
            return false;

        if (!stmt->Bind(2, fileId.m_bytes))
            return false;

        if (!stmt->Bind(3, asset.GetType().GetName()))
            return false;

        if (!stmt->Bind(4, asset.GetName()))
            return false;

        if (!stmt->Bind(5, static_cast<uint16_t>(AssetState::Unknown)))
            return false;

        const uint32_t importDataHash = FNV32::HashData(asset.GetImportData().Data(), asset.GetImportData().Size());
        if (!stmt->Bind(6, importDataHash))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::FindOne(AssetDatabase& db, const Uuid& assetId, AssetModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset WHERE id = ?
        )");

        if (!stmt->Bind(1, assetId.m_bytes))
            return false;

        const sqlite::StepResult res = stmt->Step();

        if (res == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return stmt->Step() == sqlite::StepResult::Done;
        }

        return false;
    }

    bool AssetModel::FindAll(AssetDatabase& db, const Uuid& fileId, Vector<AssetModel>& models)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset WHERE asset_file_id = ?
        )");

        if (!stmt->Bind(1, fileId.m_bytes))
            return false;

        return stmt->EachRow([&](const sqlite::Statement& stmt)
        {
            Read(stmt, models.EmplaceBack());
        });
    }

    bool AssetModel::RemoveOne(AssetDatabase& db, const Uuid& assetId)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELTE FROM asset WHERE id = ?
        )");

        if (!stmt->Bind(1, assetId.m_bytes))
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

        return stmt->Step() == sqlite::StepResult::Done;
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
            return stmt->Step() == sqlite::StepResult::Done;
        }

        return false;
    }
}
