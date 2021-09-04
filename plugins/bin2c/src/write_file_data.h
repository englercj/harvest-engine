// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

void WriteFileData(he::File& file, he::StringView name, const uint8_t* data, size_t size, bool asText);
