// Copyright Chad Engler

#include "he/assets/asset_file_scanner.h"

#include "he/assets/asset_database.h"
#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/core/allocator.h"
#include "he/core/clock_fmt.h"
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
#include "he/sqlite/orm.h"

using namespace he::sqlite;

namespace he::assets
{
    AssetFileScanner::AssetFileScanner(AssetDatabase& db) noexcept
        : m_db(db)
    {
        GetSecureRandomBytes(reinterpret_cast<uint8_t*>(&m_token), sizeof(m_token));
    }

    bool AssetFileScanner::Run(const char* rootDir)
    {
        // Set a sentinel in the database to indicate a scan is in progress.
        if (!WriteScanHeader())
            return false;

        // Scan the asset directory and update the cache db
        if (!ScanDirectory(rootDir))
            return false;

        // Remove anything from the DB that we didn't find in the scan
        if (!m_db.Storage().Destroy<AssetFileModel>(Where(Col(&AssetFileModel::scanToken) != m_token)))
            return false;

        // Remove our sentinel so other processes know they can do work.
        if (!ClearScanHeader())
            return false;

        // Wait for our pending async operations to complete.
        while (m_pendingOps)
        {
            HE_SPIN_WAIT_PAUSE();
        }

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

                    if (!m_db.IsFileUpToDate(assetFilePath))
                    {
                        auto callback = AssetDatabase::LoadDelegate::Make<&AssetFileScanner::OnUpdateComplete>(this);
                        ++m_pendingOps;
                        m_db.UpdateAssetFileAsync(assetFilePath, callback);
                    }
                    else
                    {
                        const auto query = Update(
                            Where(Col(&AssetFileModel::filePath) == fullPath),
                            Set(&AssetFileModel::scanToken, m_token));

                        if (!m_db.Storage().Execute(query))
                            return false;
                    }
                }
            }
        }

        return true;
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

    void AssetFileScanner::OnUpdateComplete(AssetDatabase::LoadResult result)
    {
        const AssetFileUuid assetFileUuid{ result.builder.Root().GetUuid() };

        const auto query = Update(
            Where(Col(&AssetFileModel::uuid) == assetFileUuid),
            Set(&AssetFileModel::scanToken, m_token));

        m_db.Storage().Execute(query);
        --m_pendingOps;
    }
}
