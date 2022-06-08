// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"

namespace he::assets
{
    class AssetDatabaseUpdater
    {
    public:
        static AssetDatabaseUpdater* Create(AssetDatabase& db);
        static void Destroy(AssetDatabaseUpdater* db);

    public:
        virtual ~AssetDatabaseUpdater() {}

    protected:
        AssetDatabaseUpdater(AssetDatabase& db);

        virtual bool Start(const char* rootDir) = 0;
        virtual bool Stop() = 0;

    protected:
        AssetDatabase& m_db;
    };
}
