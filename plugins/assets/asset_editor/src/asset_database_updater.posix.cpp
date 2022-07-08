// Copyright Chad Engler

#include "he/assets/asset_database_updater.h"

#include "he/assets/asset_file_scanner.h"
#include "he/core/types.h"

#if defined(HE_PLATFORM_API_POSIX)

namespace he::assets
{
    class AssetDatabaseUpdaterImpl final : public AssetDatabaseUpdater
    {
    public:
        AssetDatabaseUpdaterImpl(AssetDatabase& db) : AssetDatabaseUpdater(db) {}

        bool Run(const char* rootDir) override
        {
            // TODO: start file watcher first
            AssetFileScanner scanner;
            if (!scanner.Run(rootDir))
                return false;

            return false;
        }
    };

    AssetDatabaseUpdater* AssetDatabaseUpdater::Create(AssetDatabase& db)
    {
        return Allocator::GetDefault().New<AssetDatabaseUpdaterImpl>(db);
    }

    void AssetDatabaseUpdater::Destroy(AssetDatabaseUpdater* updater)
    {
        Allocator::GetDefault().Delete(db);
    }
}

#endif
