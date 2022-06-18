// Copyright Chad Engler

#pragma once

#include "he/assets/asset_types.hsc.h"

#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/types.h"
#include "he/core/uuid.h"

namespace he::assets
{
    /// The file extension used by Harvest asset files.
    constexpr StringView AssetFileExtension{ ".he_asset" };

    /// A Uuid wrapper to provide a type-safe expression of an Asset's Uuid or Asset File's Uuid.
    /// The tag template param is used to make the type unique, but the base class here exists
    /// to reduce code duplication.
    ///
    /// \internal
    /// @tparam Tag A tag type used to make the type unique.
    template <typename Tag>
    struct _UuidWrapper
    {
        _UuidWrapper() noexcept : val() {}
        _UuidWrapper(const Uuid& uuid) noexcept : val(uuid) {}
        _UuidWrapper(schema::Uuid::Reader uuid) noexcept { *this = uuid; }

        _UuidWrapper& operator=(const schema::Uuid::Reader uuid) noexcept
        {
            Span<const uint8_t> value = uuid.GetValue();
            HE_ASSERT(value.Size() == sizeof(Uuid::m_bytes));
            MemCopy(val.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
            return *this;
        }

        constexpr bool operator==(const _UuidWrapper& x) const { return val == x.val; }
        constexpr bool operator!=(const _UuidWrapper& x) const { return val != x.val; }
        constexpr bool operator<(const _UuidWrapper& x) const { return val < x.val; }

        inline bool operator==(const schema::Uuid::Reader x) const
        {
            Span<const uint8_t> value = x.GetValue();
            return value.Size() == sizeof(Uuid::m_bytes) && MemEqual(val.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
        }

        inline bool operator!=(const schema::Uuid::Reader x) const
        {
            Span<const uint8_t> value = x.GetValue();
            return value.Size() != sizeof(Uuid::m_bytes) || !MemEqual(val.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
        }

        inline bool operator<(const schema::Uuid::Reader x) const
        {
            Span<const uint8_t> value = x.GetValue();
            return value.Size() == sizeof(Uuid::m_bytes) && MemLess(val.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
        }

        Uuid val;
    };

    /// Tag used to make _UuidWrapper unique for AssetUuid.
    /// \internal
    struct AssetUuidTag;

    /// Tag used to make _UuidWrapper unique for AssetFileUuid.
    /// \internal
    struct AssetFileUuidTag;

    /// A Uuid wrapper to provide a type-safe expression of an Asset's Uuid.
    /// This type also has formatters for consistent printing.
    using AssetUuid = _UuidWrapper<AssetUuidTag>;

    /// A Uuid wrapper to provide a type-safe expression of an Asset File's Uuid.
    /// This type also has formatters for consistent printing.
    using AssetFileUuid = _UuidWrapper<AssetFileUuidTag>;

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

    template <typename T>
    struct hash<he::assets::_UuidWrapper<T>>
    {
        size_t operator()(const he::assets::_UuidWrapper<T>& value) const
        {
            return std::hash<he::Uuid>()(value.val);
        }
    };
}
