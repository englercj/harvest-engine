// Copyright Chad Engler

#include "he/assets/asset_file_scanner.h"

#include "he/assets/asset_database.h"
#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/core/allocator.h"
#include "he/core/directory.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_view.h"

#include "capnp/message.h"
#include "capnp/serialize.h"

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
        Directory::Scanner scanner;

        if (!scanner.Open(dir))
            return false;

        const uint32_t dirLen = String::Length(dir);
        String fullPath(Allocator::GetTemp());

        Directory::Scanner::Entry entry;
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

            const uint32_t expectedBytes = pending.words.Size() * sizeof(capnp::word);
            if (!r.result || r.bytesTransferred != expectedBytes)
                return false;

            if (!ProcessFile(pending.words))
                return false;

            if (max != 0 && ++done >= max)
                break;
        }

        return true;
    }

    bool AssetFileScanner::ProcessFile(Span<const capnp::word> words)
    {
        const kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments{ { words.Data(), words.Size() } };
        capnp::SegmentArrayMessageReader reader(segments);
        AssetFile::Reader file = reader.getRoot<AssetFile>();

        AssetFileModel model;
        return AssetFileModel::AddOrUpdate(m_db, file, model);
    }

    bool AssetFileScanner::ReadFile(const char* fname)
    {
        PendingLoad* pending = FindAvailablePending();

        if (!pending)
            ProcessPending(1, true);

        pending = FindAvailablePending();
        HE_ASSERT(pending);

        if (!pending->file.Open(fname, FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan))
            return false;

        const uint64_t fileSize = pending->file.GetSize();

        if (fileSize > std::numeric_limits<uint32_t>::max())
            return false;

        if ((fileSize % sizeof(capnp::word)) != 0)
            return false;

        const uint32_t fileByteSize = static_cast<uint32_t>(fileSize);
        const uint32_t wordCount = fileByteSize / sizeof(capnp::word);

        pending->words.Resize(wordCount);
        pending->load = pending->file.ReadAsync(pending->words.Data(), 0, fileByteSize);
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
