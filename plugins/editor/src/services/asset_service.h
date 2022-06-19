// Copyright Chad Engler

#pragma once

#include "file_loader_service.h"
#include "project_service.h"

#include "he/assets/asset_database.h"
#include "he/assets/asset_database_updater.h"
#include "he/core/types.h"

namespace he::editor
{
    constexpr const char AssetDbFile[] = "asset_cache.db";

    class AssetService
    {
    public:
        AssetService(
            FileLoaderService& fileLoaderService,
            ProjectService& projectService);

        bool Initialize();
        void Terminate();

        assets::AssetDatabase& AssetDB() { return m_db; }
        const assets::AssetDatabase& AssetDB() const { return m_db; }

    private:
        void OnProjectLoaded();

    private:
        FileLoaderService& m_fileLoaderService;
        ProjectService& m_projectService;

        assets::AssetDatabase m_db{};
        assets::AssetDatabaseUpdater* m_updater{ nullptr };
    };
}
