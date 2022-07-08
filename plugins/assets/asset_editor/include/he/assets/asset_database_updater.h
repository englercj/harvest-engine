// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"

namespace he::assets
{
    class AssetDatabaseUpdater
    {
    public:
        static AssetDatabaseUpdater* Create(AssetDatabase& db);
        static void Destroy(AssetDatabaseUpdater* updater);

    public:
        virtual ~AssetDatabaseUpdater() = default;

        virtual bool Start() = 0;
        virtual void Stop() = 0;

    protected:
        AssetDatabaseUpdater(AssetDatabase& db) : m_db(db) {}

    protected:
        AssetDatabase& m_db;
    };
}
