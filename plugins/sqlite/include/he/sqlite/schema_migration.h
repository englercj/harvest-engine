// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he::sqlite
{
    struct SchemaMigration
    {
        const char* description = nullptr;
        const char* sql = nullptr;
    };
}
