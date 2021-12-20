// Copyright Chad Engler

#include "he/assets/asset_database.h"

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
}
