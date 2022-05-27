// Copyright Chad Engler

#pragma once

#include "he/assets/asset_types.hsc.h"

#include "he/core/memory_ops.h"
#include "he/core/types.h"
#include "he/core/uuid.h"

namespace he::assets
{
    constexpr StringView AssetFileExtension{ ".he_asset" };

    // The type of an asset processor.
    enum class ProcessorType : uint8_t
    {
        Unknown,
        Importer,
        Compiler,
    };

    enum class CompilerReferenceType : uint8_t
    {
        AssetData,
        AssetBackingFile,
        FilePath,
        Resource,
    };

    inline Uuid ToUuid(AssetId::Reader assetId)
    {
        Span<const uint8_t> value = assetId.GetValue();
        HE_ASSERT(value.Size() == sizeof(Uuid::m_bytes));

        Uuid id;
        MemCopy(id.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
        return id;
    }

    inline Uuid ToUuid(AssetFileId::Reader fileId)
    {
        Span<const uint8_t> value = fileId.GetValue();
        HE_ASSERT(value.Size() == sizeof(Uuid::m_bytes));

        Uuid id;
        MemCopy(id.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
        return id;
    }

    // An asset type id, which is the hash of an asset type name.
    //HE_DEFINE_KEY_TYPE(AssetTypeId, uint32_t, 0);

    //HE_DEFINE_KEY_TYPE(ImporterId, uint32_t, 0);
    //HE_DEFINE_KEY_TYPE(ImporterVersion, uint32_t, 0);

    //HE_DEFINE_KEY_TYPE(CompilerId, uint32_t, 0);
    //HE_DEFINE_KEY_TYPE(CompilerVersion, uint32_t, 0);
}

// Hash overloads
namespace std
{
    template <typename> struct hash;

    template <>
    struct hash<he::assets::AssetId::Reader>
    {
        size_t operator()(const he::assets::AssetId::Reader& value) const;
    };

    template <>
    struct hash<he::assets::AssetFileId::Reader>
    {
        size_t operator()(const he::assets::AssetFileId::Reader& value) const;
    };
}
