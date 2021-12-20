// Copyright Chad Engler

#pragma once

#include "he/assets/asset_types.capnp.h"

#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/uuid.h"

namespace he::assets
{
    constexpr StringView AssetFileExtension{ ".he_asset" };

    struct AssetId
    {
        Uuid val;
    };

    inline bool operator==(const AssetId& a, const AssetId& b) { return a.val == b.val; }
    inline bool operator!=(const AssetId& a, const AssetId& b) { return a.val != b.val; }

    struct AssetFileId
    {
        Uuid val;
    };

    inline bool operator==(const AssetFileId& a, const AssetFileId& b) { return a.val == b.val; }
    inline bool operator!=(const AssetFileId& a, const AssetFileId& b) { return a.val != b.val; }

    inline bool operator==(const Uuid& id, capnp::Data::Reader data)
    {
        HE_ASSERT(data.size() == sizeof(id.m_bytes));
        return MemEqual(id.m_bytes, data.begin(), data.size());
    }

    // The name of an asset type. Expected to be unique from any other asset type's name.
    struct AssetTypeName
    {
        String name;
    };

    // An asset type id, which is the hash of an asset type name.
    //HE_DEFINE_KEY_TYPE(AssetTypeId, uint32_t, 0);

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
    struct hash<he::assets::AssetId>
    {
        size_t operator()(const he::assets::AssetId& value) const { return hash<he::Uuid>()(value.val); }
    };

    template <>
    struct hash<he::assets::AssetFileId>
    {
        size_t operator()(const he::assets::AssetFileId& value) const { return hash<he::Uuid>()(value.val); }
    };
}
