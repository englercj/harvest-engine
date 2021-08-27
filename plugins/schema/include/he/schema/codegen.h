// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/schema/code_writer.h"
#include "he/schema/schema.h"

namespace he::schema
{
    struct CodeGenOptions
    {
    };

    bool GenerateCpp(const SchemaDef& def, const CodeGenOptions& options, CodeWriter& writer);
}
