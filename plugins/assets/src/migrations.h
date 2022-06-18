// Copyright Chad Engler

#pragma once

#include "he/sqlite/schema_migration.h"

#include "migrations/001_initial.sql.h"

namespace he::assets
{
    static const sqlite::SchemaMigration AssetDatabase_Migrations[] =
    {
        { "Initial Schema", c_001_initial_sql },
    };
}
