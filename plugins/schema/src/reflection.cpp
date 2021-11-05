// Copyright Chad Engler

#include "he/schema/reflection.h"

namespace he::schema
{
    Database& Database::Get()
    {
        static Database s_database;
        return s_database;
    }
}
