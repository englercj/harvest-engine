// Copyright Chad Engler

#include "he/assets/asset_models.h"

#include "he/assets/asset_database.h"
#include "he/assets/types_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/hash.h"
#include "he/core/hash_table.h"
#include "he/core/uuid_fmt.h"
#include "he/sqlite/database.h"
#include "he/sqlite/statement.h"

namespace he::assets
{
    static void Read(const sqlite::Statement& stmt, AssetFileModel& model)
    {
        // column 0: id
        stmt.GetColumn(1).ReadBlob(model.uuid.val.m_bytes);
        model.file.path = stmt.GetColumn(2).GetText();
        // column 3: file_path_depth
        model.file.writeTime.val = BitCast<uint64_t>(stmt.GetColumn(4).GetInt64());
        model.file.size = stmt.GetColumn(5).GetUint();
        model.source.path = stmt.GetColumn(6).GetText();
        model.source.writeTime.val = BitCast<uint64_t>(stmt.GetColumn(7).GetInt64());
        model.source.size = stmt.GetColumn(8).GetUint();
    }

    static void Read(const sqlite::Statement& stmt, AssetModel& model)
    {
        // column 0: id
        stmt.GetColumn(1).ReadBlob(model.uuid.val.m_bytes);
        model.fileId = stmt.GetColumn(2).GetUint();
        model.type = stmt.GetColumn(3).GetText();
        model.name = stmt.GetColumn(4).GetText();
        model.state = AssetState(stmt.GetColumn(5).GetUint());
        model.dataHash = stmt.GetColumn(6).GetUint();
        model.importDataHash = stmt.GetColumn(7).GetUint();
        model.importerId = stmt.GetColumn(8).GetUint();
        model.importerVersion = stmt.GetColumn(9).GetUint();
        model.compilerId = stmt.GetColumn(10).GetUint();
        model.compilerVersion = stmt.GetColumn(11).GetUint();
    }

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

    bool AssetFileModel::AddOrUpdate(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model)
    {
        HE_ASSERT(model.uuid == file.GetUuid());

        sqlite::Transaction transaction = db.BeginTransaction();

        AssetFileUuid ids[2];
        String paths[2];
        uint32_t count = 0;

        // Read existing items to see what kind of operation we need to perform.
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT uuid, file_path FROM asset_file WHERE uuid = ? OR file_path = ?
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
                    (uuid, file_path, file_path_depth, file_write_time, file_size, source_path, source_write_time, source_size)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT (uuid) DO UPDATE SET
                    file_path = excluded.file_path,
                    file_path_depth = excluded.file_path_depth,
                    file_write_time = excluded.file_write_time,
                    file_size = excluded.file_size,
                    source_path = excluded.source_path,
                    source_write_time = excluded.source_write_time,
                    source_size = excluded.source_size
            )");

            if (!stmt->Bind(1, model.uuid.val.m_bytes))
                return false;

            if (!stmt->Bind(2, model.file.path))
                return false;

            if (!stmt->Bind(3, GetPathDepth(model.file.path)))
                return false;

            if (!stmt->Bind(4, BitCast<int64_t>(model.file.writeTime.val)))
                return false;

            if (!stmt->Bind(5, model.file.size))
                return false;

            if (!stmt->Bind(6, model.source.path))
                return false;

            if (!stmt->Bind(7, BitCast<int64_t>(model.source.writeTime.val)))
                return false;

            if (!stmt->Bind(8, model.source.size))
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
        HashSet<AssetUuid> assetUuids;
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
                assetUuids.Insert(assetId);
            });

            if (!res)
                return false;
        }

        // Add or update each existing asset in the file
        for (const Asset::Reader asset : file.GetAssets())
        {
            AssetModel::AddOrUpdate(db, model.uuid, asset);

            const AssetUuid assetId(asset.GetUuid());
            assetUuids.Erase(assetId);
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

    bool AssetFileModel::FindOne(AssetDatabase& db, StringView path, AssetFileModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE file_path = ?
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

    bool AssetFileModel::FindOne(AssetDatabase& db, uint32_t fileId, AssetFileModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE id = ?
        )");

        if (!stmt->Bind(1, fileId))
            return false;

        if (stmt->Step() == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
        }

        return false;
    }

    bool AssetFileModel::FindOne(AssetDatabase& db, const AssetUuid& assetUuid, AssetFileModel& model)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE id = (SELECT asset_file_id FROM asset WHERE uuid = ?)
        )");

        if (!stmt->Bind(1, assetUuid.val.m_bytes))
            return false;

        if (stmt->Step() == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
        }

        return false;
    }

    bool AssetFileModel::FindOne(AssetDatabase& db, StringView source, AssetFileModel& model, AssetFileSourcePathTag)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE source_path = ?
        )");

        if (!stmt->Bind(1, source))
            return false;

        if (stmt->Step() == sqlite::StepResult::Row)
        {
            Read(*stmt, model);
            return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
        }

        return false;
    }

    bool AssetFileModel::FindAll(AssetDatabase& db, StringView pathPrefix, Vector<AssetFileModel>& models)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            SELECT * FROM asset_file WHERE file_path_depth = ? AND file_path LIKE ? || '%'
        )");

        if (!stmt->Bind(1, GetPathDepth(pathPrefix)))
            return false;

        if (!stmt->Bind(2, pathPrefix))
            return false;

        return stmt->EachRow([&](const sqlite::Statement& stmt)
        {
            Read(stmt, models.EmplaceBack());
        });
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

    bool AssetFileModel::RemoveOne(AssetDatabase& db, StringView path)
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

    bool AssetFileModel::UpdateScanToken(AssetDatabase& db, StringView path, uint32_t scanToken)
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

        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset
                (uuid, asset_file_id, asset_type_name, name, state, data_hash, import_data_hash, importer_id, importer_version, compiler_id, compiler_version)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT (uuid) DO UPDATE SET
                asset_file_id = excluded.asset_file_id,
                asset_type_name = excluded.asset_type_name,
                name = excluded.name,
                state = excluded.state,
                data_hash = excluded.data_hash,
                import_data_hash = excluded.import_data_hash,
                importer_id = excluded.importer_id,
                importer_version = excluded.importer_version,
                compiler_id = excluded.compiler_id,
                compiler_version = excluded.compiler_version
        )");

        if (!stmt->Bind(1, model.uuid.val.m_bytes))
            return false;

        if (!stmt->Bind(2, model.fileId))
            return false;

        if (!stmt->Bind(3, model.type))
            return false;

        if (!stmt->Bind(4, model.name))
            return false;

        if (!stmt->Bind(5, AsUnderlyingType(model.state)))
            return false;

        if (!stmt->Bind(6, model.dataHash))
            return false;

        if (!stmt->Bind(7, model.importDataHash))
            return false;

        if (!stmt->Bind(8, model.importerId))
            return false;

        if (!stmt->Bind(9, model.importerVersion))
            return false;

        if (!stmt->Bind(10, model.compilerId))
            return false;

        if (!stmt->Bind(11, model.compilerVersion))
            return false;

        return stmt->Step() == sqlite::StepResult::Done;
    }

    bool AssetModel::AddOrUpdate(AssetDatabase& db, const AssetFileUuid& fileUuid, Asset::Reader asset)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            INSERT INTO asset
                (uuid, asset_file_id, asset_type_name, name, state, data_hash, import_data_hash)
            SELECT ?, asset_file.id, ?, ?, ?, ?, ?
            FROM asset_file WHERE asset_file.uuid = ?
            ON CONFLICT (uuid) DO UPDATE SET
                asset_file_id = excluded.asset_file_id,
                asset_type_name = excluded.asset_type_name,
                name = excluded.name,
                data_hash = excluded.data_hash,
                import_data_hash = excluded.import_data_hash
        )");

        if (!stmt->Bind(1, asset.GetUuid().GetValue()))
            return false;

        if (!stmt->Bind(2, asset.GetType()))
            return false;

        if (!stmt->Bind(3, asset.GetName()))
            return false;

        if (!stmt->Bind(4, AsUnderlyingType(AssetState::Unknown)))
            return false;

        // TODO: These hashes are incorrect. If the structures contain pointers, the pointer
        // values are going to get hashed. This means the same data, but in different order,
        // will have different hashes.

        const schema::AnyStruct::Reader dataPtr = asset.GetData();
        const uint32_t dataHash = dataPtr.IsNull() ? 0 : CRC32C::Mem(dataPtr.Target(), dataPtr.StructWordSize());
        if (!stmt->Bind(5, dataHash))
            return false;

        const schema::AnyStruct::Reader importDataPtr = asset.GetImportData();
        const uint32_t importDataHash = importDataPtr.IsNull() ? 0 : CRC32C::Mem(importDataPtr.Target(), importDataPtr.StructWordSize());
        if (!stmt->Bind(6, importDataHash))
            return false;

        if (!stmt->Bind(7, fileUuid.val.m_bytes))
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

    bool AssetModel::FindAll(AssetDatabase& db, StringView search, Vector<AssetModel>& models)
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

    bool AssetModel::FindAll(AssetDatabase& db, StringView pathPrefix, Vector<AssetModel>& models, AssetFilePathTag)
    {
        // Special case handling for the root, which we can skip the LIKE clause for
        if (pathPrefix.IsEmpty() || pathPrefix == "/")
        {
            sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
                SELECT * FROM asset
                JOIN asset_file ON asset_file.id = asset.asset_file_id
                WHERE asset_file.file_path_depth = ?
            )");

            if (!stmt->Bind(1, 0))
                return false;

            return stmt->EachRow([&](const sqlite::Statement& stmt)
            {
                Read(stmt, models.EmplaceBack());
            });
        }

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
        sqlite::ScopedStatement stmt = db.StatementLiteral(hasSlash ? SqlQueryPathWithSlash : SqlQueryPathWithoutSlash);

        if (!stmt->Bind(1, GetPathDepth(pathPrefix)))
            return false;

        if (!stmt->Bind(2, pathPrefix))
            return false;

        return stmt->EachRow([&](const sqlite::Statement& stmt)
        {
            Read(stmt, models.EmplaceBack());
        });
    }

    bool AssetModel::RemoveOne(AssetDatabase& db, const AssetUuid& assetUuid)
    {
        sqlite::ScopedStatement stmt = db.StatementLiteral(R"(
            DELETE FROM asset WHERE uuid = ?
        )");

        if (!stmt->Bind(1, assetUuid.val.m_bytes))
            return false;

        return HE_VERIFY(stmt->Step() == sqlite::StepResult::Done);
    }

    bool AssetModel::AddTag(AssetDatabase& db, const AssetUuid& assetUuid, StringView tag)
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

    bool AssetModel::RemoveTag(AssetDatabase& db, const AssetUuid& assetUuid, StringView tag)
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

    bool ConfigModel::FindOne(AssetDatabase& db, StringView key, ConfigModel& outModel)
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
