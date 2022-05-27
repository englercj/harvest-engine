// Copyright Chad Engler

#include "he/assets/types.h"

#include "he/core/assert.h"
#include "he/core/span.h"
#include "he/core/memory_ops.h"
#include "he/core/types.h"

namespace std
{
    size_t hash<he::assets::AssetId::Reader>::operator()(const he::assets::AssetId::Reader& value) const
    {
        const he::Span<const uint8_t> bytes = value.GetValue();
        HE_ASSERT(bytes.Size() >= sizeof(size_t));

        size_t h;
        he::MemCopy(&h, bytes.Data(), sizeof(h));
        return h;
    }

    size_t hash<he::assets::AssetFileId::Reader>::operator()(const he::assets::AssetFileId::Reader& value) const
    {
        const he::Span<const uint8_t> bytes = value.GetValue();
        HE_ASSERT(bytes.Size() >= sizeof(size_t));

        size_t h;
        he::MemCopy(&h, bytes.Data(), sizeof(h));
        return h;
    }
}
