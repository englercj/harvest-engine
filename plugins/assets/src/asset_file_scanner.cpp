// Copyright Chad Engler

#include "he/assets/asset_file_scanner.h"

#include "he/assets/asset_database.h"
#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/core/allocator.h"
#include "he/core/directory.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/random.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view.h"
#include "he/schema/toml.h"

namespace he::assets
{
    AssetFileScanner::AssetFileScanner(AssetDatabase& db)
        : m_db(db)
    {
        GetSecureRandomBytes(reinterpret_cast<uint8_t*>(&m_token), sizeof(m_token));
    }

    bool AssetFileScanner::Run(const char* rootDir)
    {
        // Set a sentinel in the database to indicate a scan is in progress. This
        if (!WriteScanHeader())
            return false;

        // Scan the asset directory and update the cache db
        if (!ScanDirectory(rootDir))
            return false;

        // Finish the processing of files in the asset directory.
        // A false return means there was an error and it ended early, so here we loop and wait
        // again until we get a true return value indicating it completed successfully.
        bool error = false;
        while (!ProcessPending(0, true))
        {
            error = true;
        }

        if (error)
            return false;

        if (!AssetFileModel::RemoveAll(m_db, m_token))
            return false;

        if (!ClearScanHeader())
            return false;

        return true;
    }

    bool AssetFileScanner::ScanDirectory(const char* dir)
    {
        DirectoryScanner scanner;

        if (!scanner.Open(dir))
            return false;

        const uint32_t dirLen = String::Length(dir);
        String fullPath;

        DirectoryScanner::Entry entry;
        while (scanner.NextEntry(entry))
        {
            ProcessPending(0, false);
            fullPath.Assign(dir, dirLen);
            ConcatPath(fullPath, entry.name);

            if (entry.isDirectory)
            {
                if (!ScanDirectory(fullPath.Data()))
                    return false;
            }
            else
            {
                const StringView ext = GetExtension(fullPath);
                if (ext == AssetFileExtension)
                {
                    const char* assetFilePath = fullPath.Data();

                    if (!IsFileUpToDate(assetFilePath))
                        StartFileUpdate(assetFilePath);
                }
            }
        }

        return true;
    }

    bool AssetFileScanner::IsFileUpToDate(const char* path)
    {
        AssetFileModel model;
        if (!AssetFileModel::FindOne(m_db, path, model))
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
        if (model.fileSize != fileSize)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file size has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_file_size, model.fileSize),
                HE_KV(file_size, fileSize));
            return false;
        }

        if (model.writeTime != attributes.writeTime)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file write time has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_write_time, model.writeTime),
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
            if (model.source.fileSize != sourceFileSize)
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Source file size has changed, update required."),
                    HE_KV(path, path),
                    HE_KV(last_file_size, model.source.fileSize),
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

        // Nothing has changed, good to go. Just mark the scan token.
        return AssetFileModel::UpdateScanToken(m_db, model.id, m_token);
    }

    bool AssetFileScanner::StartFileUpdate(const char* fname)
    {
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

    bool AssetFileScanner::ProcessPending(uint32_t max, bool wait)
    {
        uint32_t done = 0;
        for (PendingLoad& pending : m_pending)
        {
            if (!pending.file.IsOpen() || !pending.load.valid())
                continue;

            if (pending.load.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            {
                if (!wait)
                    continue;

                pending.load.wait();
            }

            HE_AT_SCOPE_EXIT([&]() { pending.file.Close(); });

            AsyncFileResult r = pending.load.get();

            const uint32_t expectedBytes = pending.content.Size();
            if (!r.result || r.bytesTransferred != expectedBytes)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to load asset file."),
                    HE_KV(file_path, pending.path),
                    HE_KV(result, r.result),
                    HE_KV(bytes_read, r.bytesTransferred),
                    HE_KV(bytes_expected, expectedBytes));
                return false;
            }

            if (!ProcessFile(pending))
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to process asset file."),
                    HE_KV(file_path, pending.path));
                return false;
            }

            if (max != 0 && ++done >= max)
                break;
        }

        return true;
    }

    bool AssetFileScanner::ProcessFile(const PendingLoad& pending)
    {
        schema::Builder builder;
        if (!schema::FromToml<AssetFile>(builder, pending.content.Data()))
            return false;

        AssetFile::Reader assetFile = builder.Root().TryGetStruct<AssetFile>();
        if (!assetFile.IsValid())
            return false;

        FileAttributes attributes;
        Result res = pending.file.GetAttributes(attributes);
        if (!res)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to read asset file attributes."),
                HE_KV(result, res));
            return false;
        }

        AssetFileModel model;
        model.id = ToUuid(assetFile.GetId());
        model.path = pending.path;
        model.writeTime = attributes.writeTime;
        model.fileSize = static_cast<uint32_t>(attributes.size);
        model.source = {};
        model.scanToken = m_token;

        if (assetFile.HasSource() && !assetFile.GetSource().IsEmpty())
        {
            const schema::String::Reader sourcePath = assetFile.GetSource();

            model.source.path = sourcePath;

            // TODO: Modify for when we support sparse checkout of source files.
            res = File::GetAttributes(sourcePath.Data(), attributes);
            if (res)
            {
                model.source.writeTime = attributes.writeTime;
                model.source.fileSize = static_cast<uint32_t>(attributes.size);
            }
            else
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to read source file attributes."),
                    HE_KV(result, res));
            }
        }

        return AssetFileModel::AddOrUpdate(m_db, assetFile, model);
    }

    bool AssetFileScanner::WriteScanHeader()
    {
        // TODO: This header allows us to have multiple processes use the DB without fighting for
        // scan rights. Because SQLite doesn't have support for table locking, and we don't want
        // to exclusive lock the entire DB during a scan we do our own optimistic locking scheme.
        //
        // When starting a scan we check for an existing header in the config table, its existence
        // indicates a scan is already in-progress. We check that existing header to see if the pid
        // is still running. If the pid is running, we consider this header good and bail. Otherwise,
        // we write our own and start the scan.
        return true;
    }

    bool AssetFileScanner::ClearScanHeader()
    {
        // TODO!
        return true;
    }

    AssetFileScanner::PendingLoad* AssetFileScanner::FindAvailablePending()
    {
        for (PendingLoad& pending : m_pending)
        {
            if (!pending.file.IsOpen())
                return &pending;
        }

        return nullptr;
    }
}
