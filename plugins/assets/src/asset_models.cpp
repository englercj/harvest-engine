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
        if (!stmt.Bind(1, model.id.val.m_bytes))
            return false;

        if (!stmt.Bind(2, model.path.Data()))
            return false;

        if (!stmt.Bind(3, int64_t(model.lastModifiedTime.ns)))
            return false;

        if (!stmt.Bind(4, model.lastFileSize))
            return false;

        if (!stmt.Bind(5, model.lastSessionToken))
            return false;

        if (!stmt.Bind(6, int64_t(model.source.lastModifiedTime.ns)))
            return false;

        if (!stmt.Bind(7, model.source.lastFileSize))
            return false;

        return true;
    }

    static void Read(const sqlite::Statement& stmt, AssetFileModel& model)
    {
        stmt.GetColumn(0).ReadBlob(model.id.val.m_bytes);
        model.path = stmt.GetColumn(1).GetText().Data();
        model.lastModifiedTime.ns = uint64_t(stmt.GetColumn(2).GetInt64());
        model.lastFileSize = stmt.GetColumn(3).GetUint();
        model.lastSessionToken = stmt.GetColumn(4).GetUint();
        model.source.lastModifiedTime.ns = uint64_t(stmt.GetColumn(5).GetInt64());
        model.source.lastFileSize = stmt.GetColumn(6).GetUint();
    }

    static bool Bind(const sqlite::Statement& stmt, const AssetModel& model)
    {
        if (!stmt.Bind(1, model.id.val.m_bytes))
            return false;

        if (!stmt.Bind(2, model.fileId.val.m_bytes))
            return false;

        if (!stmt.Bind(3, model.type.Data()))
            return false;

        if (!stmt.Bind(4, model.name.Data()))
            return false;

        if (!stmt.Bind(5, uint32_t(model.state)))
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
        stmt.GetColumn(0).ReadBlob(model.id.val.m_bytes);
        stmt.GetColumn(1).ReadBlob(model.fileId.val.m_bytes);
        model.type = stmt.GetColumn(2).GetText().Data();
        model.name = stmt.GetColumn(3).GetText().Data();
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
        const capnp::Data::Reader fileId = file.getId();
        HE_ASSERT(model.id.val == fileId);

        const Span<const uint8_t> fileIdBytes{ fileId.begin(), fileId.end() };

        sqlite::Transaction transaction = db.BeginTransaction();

        AssetFileId ids[2];
        size_t count = 0;

        // Read existing items to see what kind of operation we need to perform.
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset_file WHERE id = ? OR path = ?
            )");

            if (!stmt->Bind(1, fileIdBytes) || !stmt->Bind(2, model.path.Data()))
                return false;

            const bool res = stmt->EachRow([&](const sqlite::Statement& stmt)
            {
                stmt.GetColumn(0).ReadBlob(ids[count++].val.m_bytes);
            });

            if (!res)
                return false;
        }
        HE_ASSERT(count <= 2, "Impossible row count from asset_file query: {}", count);

        // If we found any then a row may exists with the same path as the file we wish to
        // insert. If that is the case, we need to remove it first. An ID conflict is handled in
        // the upsert query later.
        if (count > 0)
        {
            size_t i = size_t(-1);

            if (ids[0].val == fileId)
                i = 0;
            else if (count > 1 && ids[1].val == fileId)
                i = 1;

            if (i != size_t(-1))
            {
                if (!AssetFileModel::RemoveOne(db, ids[i]))
                    return false;
            }
        }

        // Insert or update the file
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                INSERT INTO asset_file
                    (id, path, last_modified_time, last_file_size, last_session_token, source_modified_time, source_file_size)
                VALUES (?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT (id) DO UPDATE SET
                    path = excluded.path,
                    last_modified_time = excluded.last_modified_time,
                    last_file_size = excluded.last_file_size,
                    last_session_token = excluded.last_session_token
                    source_modified_time = excluded.source_modified_time,
                    source_file_size = excluded.source_file_size
            )");

            if (!Bind(*stmt, model))
                return false;

            if (stmt->Step() != sqlite::StepResult::Done)
                return false;
        }

        // Read existing Asset IDs so we can remove ones that no longer exist.
        std::unordered_set<AssetId> assetIds;
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT id FROM asset WHERE asset_file_id = ?
            )");

            if (!stmt->Bind(1, fileIdBytes))
                return false;

            const bool res = stmt->EachRow([&](const sqlite::Statement& stmt)
            {
                AssetId assetId;
                stmt.GetColumn(0).ReadBlob(assetId.val.m_bytes);
                assetIds.insert(assetId);
            });

            if (!res)
                return false;
        }

        // Add or update each existing asset in the file
        for (const Asset::Reader asset : file.getAssets())
        {
            AssetModel::AddOrUpdate(db, model.id, asset);

            const capnp::Data::Reader assetIdData = asset.getId();
            HE_ASSERT(assetIdData.size() == sizeof(Uuid::m_bytes));

            AssetId assetId;
            MemCopy(assetId.val.m_bytes, assetIdData.begin(), assetIdData.size());
            assetIds.erase(assetId);
        }

        // Remove each asset that no longer exists in the file.
        for (const AssetId& id : assetIds)
        {
            AssetModel::RemoveOne(db, id);
        }

        transaction.Commit();
        return true;
    }

    bool AssetFileModel::FindOne(AssetDatabase& db, const AssetFileId& id, AssetFileModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT id, path, last_modified_time, last_file_size, last_session_token, source_modified_time, source_file_size
            FROM asset_file WHERE id = ?
        )");

        if (!stmt->Bind(1, id.val.m_bytes))
            return false;

        if (stmt->Step() == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return true;
        }

        return false;
    }

    bool AssetFileModel::RemoveOne(AssetDatabase& db, const AssetFileId& id)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELETE FROM asset_file WHERE id = ?
        )");

        if (!stmt->Bind(1, id.val.m_bytes))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::AddOrUpdate(AssetDatabase& db, const AssetModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset_file
                (id, asset_file_id, type, name, state, data_hash, import_data_hash,
                importer_id, importer_version, compiler_id, compiler_version)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT (id) DO UPDATE SET
                asset_file_id = excluded.asset_file_id,
                type = excluded.type,
                name = excluded.name,
                state = excluded.state,
                data_hash = excluded.data_hash,
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

    bool AssetModel::AddOrUpdate(AssetDatabase& db, const AssetFileId& fileId, Asset::Reader asset)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset_file
                (id, asset_file_id, type, name, state, data_hash, import_data_hash)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT (id) DO UPDATE SET
                asset_file_id = excluded.asset_file_id,
                type = excluded.type,
                name = excluded.name,
                data_hash = excluded.data_hash,
                import_data_hash = excluded.import_data_hash
        )");

        const capnp::Data::Reader assetIdData = asset.getId();
        const Span<const uint8_t> assetIdBytes{ assetIdData.begin(), assetIdData.end() };

        if (!stmt->Bind(1, assetIdBytes))
            return false;

        if (!stmt->Bind(2, fileId.val.m_bytes))
            return false;

        if (!stmt->Bind(3, asset.getType().cStr()))
            return false;

        if (!stmt->Bind(4, asset.getName().cStr()))
            return false;

        if (!stmt->Bind(5, uint16_t(AssetState::UNKNOWN)))
            return false;

        if (!stmt->Bind(6, FnvHash32(asset.data()->data(), asset.data()->size())))
            return false;

        if (!stmt->Bind(7, FnvHash32(asset.import_data()->data(), asset.import_data()->size())))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::FindOne(AssetDatabase& db, const AssetId& id, AssetModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset WHERE id = ?
        )");

        if (!stmt->Bind(0, id.val.m_bytes))
            return false;

        const sqlite::StepResult res = stmt->Step();

        if (res == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return stmt->Step() == sqlite::StepResult::Done;
        }

        return false;
    }

    bool AssetModel::FindAll(AssetDatabase& db, const AssetFileId& fileId, Vector<AssetModel>& models)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset WHERE asset_file_id = ?
        )");

        if (!stmt->Bind(0, fileId.val.m_bytes))
            return false;

        return stmt->EachRow([&](const sqlite::Statement& stmt)
        {
            Read(stmt, models.EmplaceBack());
        });
    }

    bool AssetModel::RemoveOne(AssetDatabase& db, const AssetId& id)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELTE FROM asset WHERE id = ?
        )");

        if (!stmt->Bind(0, id.val.m_bytes))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }
}
