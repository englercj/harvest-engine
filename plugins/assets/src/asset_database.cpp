// Copyright Chad Engler

#include "he/assets/asset_database.h"

#include "he/assets/asset_models.h"
#include "he/core/file.h"
#include "he/core/log.h"

#include "migrations.h"

namespace he::assets
{
    bool AssetDatabase::Initialize(const char* dbPath)
    {
        if (!m_db.Open(dbPath))
            return false;

        if (!m_db.MigrateSchema(AssetDatabase_Migrations))
            return false;

        return true;
    }

    bool AssetDatabase::Terminate()
    {
        return m_db.Close();
    }

    bool AssetDatabase::IsFileUpToDate(const char* path)
    {
        AssetFileModel model;
        if (!AssetFileModel::FindOne(*this, path, model))
            return false;

        // Check the asset file's attributes to see if it has changed since last time we've scanned it
        FileAttributes attributes;
        Result res = File::GetAttributes(path, attributes);
        if (!res)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to read asset file attributes. Assuming update is required."),
                HE_KV(path, path),
                HE_KV(result, res));
            return false;
        }

        const uint32_t fileSize = static_cast<uint32_t>(attributes.size);
        if (model.file.size != fileSize)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file size has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_file_size, model.file.size),
                HE_KV(file_size, fileSize));
            return false;
        }

        if (model.file.writeTime != attributes.writeTime)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file write time has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_write_time, model.file.writeTime),
                HE_KV(write_time, attributes.writeTime));
            return false;
        }

        // Check the asset file's source to see if it has changed since last time we've scanned it
        // If this is true, we probably want to re-run the importer to ensure import resources exist.
        if (!model.source.path.IsEmpty())
        {
            // TODO: Modify for when we support sparse checkout of source files.
            res = File::GetAttributes(model.source.path.Data(), attributes);
            if (!res)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to read source file attributes. Assuming update is not required."),
                    HE_KV(path, path),
                    HE_KV(result, res));
                return true;
            }

            const uint32_t sourceFileSize = static_cast<uint32_t>(attributes.size);
            if (model.source.size != sourceFileSize)
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Source file size has changed, update required."),
                    HE_KV(path, path),
                    HE_KV(last_file_size, model.source.size),
                    HE_KV(file_size, sourceFileSize));
                return false;
            }

            if (model.source.writeTime != attributes.writeTime)
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Source file write time has changed, update required."),
                    HE_KV(path, path),
                    HE_KV(last_write_time, model.source.writeTime),
                    HE_KV(write_time, attributes.writeTime));
                return false;
            }
        }

        // Nothing has changed, good to go.
        return true;
    }

    bool AssetDatabase::MaybeUpdateFile(const char* path)
    {
        if (IsFileUpToDate(path))
            return false;

        PendingLoad* pending = FindAvailablePending();

        if (!pending)
            ProcessPending(1, true);

        pending = FindAvailablePending();
        HE_ASSERT(pending);

        pending->path = fname;

        Result r = pending->file.Open(fname, FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan);
        if (!r)
        {
            HE_LOG_ERROR(he_assets, HE_MSG("Failed to open asset file."), HE_KV(file_path, pending->path), HE_KV(result, r));
            return false;
        }

        const uint64_t fileSize = pending->file.GetSize();

        if (fileSize > std::numeric_limits<uint32_t>::max())
        {
            HE_LOG_ERROR(he_assets, HE_MSG("Asset file size is larger than UINT32_MAX"), HE_KV(file_path, pending->path));
            return false;
        }

        const uint32_t fileByteSize = static_cast<uint32_t>(fileSize);

        pending->content.Resize(fileByteSize);
        pending->load = pending->file.ReadAsync(pending->content.Data(), 0, fileByteSize);
        return true;
    }

    void AssetDatabase::PollPendingLoads()
    {

    }

    AssetDatabase::PendingLoad* AssetDatabase::FindAvailablePending()
    {
        for (PendingLoad& pending : m_pending)
        {
            if (!pending.file.IsOpen())
                return &pending;
        }

        return nullptr;
    }
}
