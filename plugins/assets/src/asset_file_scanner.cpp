// Copyright Chad Engler

#include "he/assets/asset_file_scanner.h"

#include "he/assets/asset_database.h"
#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/core/allocator.h"
#include "he/core/directory.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view.h"
#include "he/schema/toml.h"

namespace he::assets
{
    bool AssetFileScanner::Run(const char* rootDir)
    {
        if (!ScanDirectory(rootDir))
            return false;

        return ProcessPending(0, true);
    }

    bool AssetFileScanner::ScanDirectory(const char* dir)
    {
        DirectoryScanner scanner;

        if (!scanner.Open(dir))
            return false;

        const uint32_t dirLen = String::Length(dir);
        String fullPath(Allocator::GetTemp());

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
                    ReadFile(fullPath.Data());
            }
        }

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

            pending.file.Close();

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

            FileAttributes attributes;
            Result res = pending.file.GetAttributes(attributes);
            if (!res)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to read asset file attributes."),
                    HE_KV(result, res));
                return false;
            }

            if (!ProcessFile(attributes, pending.path, pending.content))
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

    bool AssetFileScanner::ProcessFile(const FileAttributes& attributes, const String& path, const String& content)
    {
        schema::Builder builder;
        if (!schema::FromToml<AssetFile>(builder, content.Data()))
            return false;

        AssetFile::Reader assetFile = builder.Root().TryGetStruct<AssetFile>();
        if (!assetFile.IsValid())
            return false;

        AssetFileModel model;
        model.id = ToUuid(assetFile.GetId());
        model.lastSessionToken = 0; // TODO!
        model.path = path;
        model.lastModifiedTime = attributes.writeTime;
        model.lastFileSize = static_cast<uint32_t>(attributes.size);
        model.source = {}; // TODO!

        return AssetFileModel::AddOrUpdate(m_db, assetFile, model);
    }

    bool AssetFileScanner::ReadFile(const char* fname)
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
