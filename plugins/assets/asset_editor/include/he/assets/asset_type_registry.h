// Copyright Chad Engler

#pragma once

#include "he/assets/asset_importer.h"
#include "he/assets/asset_compiler.h"
#include "he/assets/types.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/core/utils.h"
#include "he/schema/types.h"

#include <unordered_map>

namespace he::assets
{
    // --------------------------------------------------------------------------------------------

    /// Registry of asset types, compilers, and importers.
    class AssetTypeRegistry
    {
    public:
        struct AssetTypeEntry
        {
            const he::schema::DeclInfo* declInfo;

            TypeInfo compilerInfo;
            UniquePtr<AssetCompiler> compiler;
        };

        struct ImporterEntry
        {
            TypeInfo importerInfo;
            UniquePtr<AssetImporter> importer;
        };

    public:
        static AssetTypeRegistry& Get();

        template <typename T>
        bool RegisterImporter();

        template <typename T>
        void UnregisterImporter();

        template <typename T>
        const ImporterEntry* FindImporter() { return FindImporter(TypeInfo::Get<T>()); }

        Span<const ImporterEntry> Importers() const { return m_importers; }

        template <typename AssetType, typename CompilerType>
        bool RegisterAssetType();

        template <typename AssetType>
        void UnregisterAssetType();

        const AssetTypeEntry* FindAssetType(AssetTypeId typeId) const { auto it = m_assetTypes.find(typeId); return it == m_assetTypes.end() ? nullptr : &it->second; }
        const AssetTypeEntry& GetAssetType(AssetTypeId typeId) const { return m_assetTypes.at(typeId); }
        AssetCompiler* FindCompiler(AssetTypeId AssetTypeId) const;

    private:
        const ImporterEntry* FindImporter(const TypeInfo& info);

        bool RegisterImporter(const TypeInfo& info, UniquePtr<AssetImporter> importer);
        void UnregisterImporter(const TypeInfo& info);

        bool RegisterAssetType(const char* assetTypeName, const he::schema::DeclInfo& declInfo, const TypeInfo& compilerInfo, UniquePtr<AssetCompiler> compiler);
        void UnregisterAssetType(const char* assetTypeName);

    private:
        Vector<ImporterEntry> m_importers;
        std::unordered_map<AssetTypeId, AssetTypeEntry> m_assetTypes;
    };

    // --------------------------------------------------------------------------------------------
    // Inline implementations

    template <typename T>
    inline bool AssetTypeRegistry::RegisterImporter()
    {
        return RegisterImporter(TypeInfo::Get<T>(), MakeUnique<T>());
    }

    template <typename T>
    inline void AssetTypeRegistry::UnregisterImporter()
    {
        UnregisterImporter(TypeInfo::Get<T>());
    }

    template <typename AssetType, typename CompilerType>
    inline bool AssetTypeRegistry::RegisterAssetType()
    {
        constexpr const he::schema::DeclInfo& DeclInfo = AssetType::DeclInfo;
        constexpr const TypeInfo& CompilerInfo = TypeInfo::Get<CompilerType>();
        return RegisterAssetType(AssetType::AssetTypeName, DeclInfo, CompilerInfo, MakeUnique<CompilerType>());
    }

    template <typename AssetType>
    inline void AssetTypeRegistry::UnregisterAssetType()
    {
        return UnregisterAssetType(AssetType::AssetTypeName);
    }
}
