// Copyright Chad Engler

#pragma once

#include "he/assets/asset_types.hsc.h"

#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/uuid.h"
#include "he/schema/schema.h"

namespace he::assets
{
    /// The file extension used by Harvest asset files.
    inline constexpr StringView AssetFileExtension{ ".he_asset" };

    /// A Uuid wrapper to provide a type-safe expression of an Asset's Uuid or Asset File's Uuid.
    /// The tag template param is used to generate a unique type, but is otherwise unused.
    ///
    /// \internal
    /// @tparam Tag A tag type used to make the type unique.
    template <typename Tag>
    struct _UuidWrapper
    {
        _UuidWrapper() noexcept : val() {}
        _UuidWrapper(const Uuid& uuid) noexcept : val(uuid) {}
        _UuidWrapper(schema::Uuid::Builder uuid) noexcept { *this = uuid.AsReader(); }
        _UuidWrapper(schema::Uuid::Reader uuid) noexcept { *this = uuid; }

        _UuidWrapper& operator=(const schema::Uuid::Reader uuid) noexcept
        {
            Span<const uint8_t> value = uuid.GetValue();
            HE_ASSERT(value.Size() == sizeof(Uuid::m_bytes));
            MemCopy(val.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
            return *this;
        }

        constexpr bool operator==(const _UuidWrapper& x) const { return val == x.val; }
        constexpr bool operator<(const _UuidWrapper& x) const { return val < x.val; }

        inline bool operator==(const schema::Uuid::Reader& x) const
        {
            Span<const uint8_t> value = x.GetValue();
            return value.Size() == sizeof(Uuid::m_bytes) && MemEqual(val.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
        }
        inline bool operator==(const schema::Uuid::Builder& x) const { return *this == x.AsReader(); }

        inline bool operator<(const schema::Uuid::Reader& x) const
        {
            Span<const uint8_t> value = x.GetValue();
            return value.Size() == sizeof(Uuid::m_bytes) && MemLess(val.m_bytes, value.Data(), sizeof(Uuid::m_bytes));
        }
        inline bool operator<(const schema::Uuid::Builder& x) const { return *this < x.AsReader(); }

        [[nodiscard]] uint64_t HashCode() const noexcept { return val.HashCode(); }

        Uuid val;
    };

    /// A Uuid wrapper to provide a type-safe expression of an Asset's Uuid.
    /// This type also has formatters for consistent printing.
    using AssetUuid = _UuidWrapper<struct AssetUuidTag>;

    /// A Uuid wrapper to provide a type-safe expression of an Asset File's Uuid.
    /// This type also has formatters for consistent printing.
    using AssetFileUuid = _UuidWrapper<struct AssetFileUuidTag>;

    /// A class to define a unique identifier that is built from the hash of a unique string.
    ///
    /// \internal
    /// @tparam Tag A tag type used to make the type unique.
    template <typename Tag>
    struct _HashId
    {
        constexpr _HashId() : val(0) {}
        constexpr explicit _HashId(uint32_t v) : val(v) {}
        constexpr explicit _HashId(const char* s) : val(FNV32::String(s)) {}
        constexpr explicit _HashId(StringView s) : val(FNV32::String(s)) {}

        constexpr bool operator==(const _HashId& x) const { return val == x.val; }
        constexpr bool operator!=(const _HashId& x) const { return val != x.val; }
        constexpr bool operator<(const _HashId& x) const { return val < x.val; }

        constexpr _HashId operator^(const _HashId& x) const { return CombineHash32(val, x.val); }

        [[nodiscard]] constexpr uint64_t HashCode() const noexcept { return val; }

        uint32_t val;
    };

    /// Unique identifier for an asset importer class. Usually a hash of a unique string.
    /// \see AssetImporter::Id()
    using ImporterId = _HashId<struct ImporterIdTag>;

    /// Version number for an asset importer class. This value is used to determine if an imported
    /// asset has been imported with the current version of the importer code.
    /// \see AssetImporter::Version()
    using ImporterVersion = _HashId<struct ImporterVersionTag>;

    /// Unique identifier for an asset compiler class. Usually a hash of a unique string.
    /// \see AssetCompiler::Id()
    using CompilerId = _HashId<struct CompilerIdTag>;

    /// Version number for an asset compiler class. This value is used to determine if an asset
    /// has been compiled with the current version of the compiler code.
    /// \see AssetCompiler::Version()
    using CompilerVersion = _HashId<struct CompilerVersionTag>;

    /// Unique identifier for a type of resource. Usually a hash of a unique string.
    using ResourceId = _HashId<struct ResourceIdTag>;
}
