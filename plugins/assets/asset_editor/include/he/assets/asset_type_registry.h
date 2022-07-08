// Copyright Chad Engler

#pragma once

#include "he/assets/asset_importer.h"
#include "he/assets/asset_compiler.h"
#include "he/assets/types.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/core/utils.h"
#include "he/schema/types.h"

#include <unordered_map>

namespace he::assets
{
    class AssetTypeRegistry
    {
    public:
        struct Entry
        {
            AssetTypeId typeId;
            const he::schema::DeclInfo* declInfo;
            AssetCompiler* compiler;
        };

    public:
        static AssetTypeRegistry& Get();

        template <typename T>
        void RegisterImporter();

        template <typename AssetType, typename CompilerType>
        bool RegisterAssetType();

        const Entry* FindAssetType(AssetTypeId typeId) const { auto it = m_assetTypes.find(typeId); return it == m_assetTypes.end() ? nullptr : &it->second; }
        const Entry& GetAssetType(AssetTypeId typeId) const { return m_assetTypes.at(typeId); }

    private:
        bool RegisterAssetType(const he::schema::DeclInfo& declInfo, UniquePtr<AssetCompiler> compiler);

    private:
        Vector<UniquePtr<AssetCompiler>> m_compilers;
        Vector<UniquePtr<AssetImporter>> m_importers;

        std::unordered_map<AssetTypeId, Entry> m_assetTypes;
    };

    // --------------------------------------------------------------------------------------------

    template <typename T>
    inline void AssetTypeRegistry::RegisterImporter()
    {
        UniquePtr<AssetImporter> importer = MakeUnique<T>();
        m_importers.PushBack(Move(importer));
    }

    template <typename AssetType, typename CompilerType>
    inline bool AssetTypeRegistry::RegisterAssetType()
    {
        const he::schema::DeclInfo& declInfo = AssetType::DeclInfo;
        return RegisterAssetType(declInfo, MakeUnique<CompilerType>());
    }
}
