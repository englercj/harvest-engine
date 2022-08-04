// Copyright Chad Engler

#include "he/assets/asset_type_registry.h"

#include "he/schema/schema.h"
#include "he/core/string_view_fmt.h"

#include <algorithm>

namespace he::assets
{
    AssetTypeRegistry& AssetTypeRegistry::Get()
    {
        static AssetTypeRegistry s_registry;
        return s_registry;
    }

    AssetCompiler* AssetTypeRegistry::FindCompiler(AssetTypeId assetTypeId) const
    {
        const auto it = m_assetTypes.find(assetTypeId);
        return it == m_assetTypes.end() ? nullptr : it->second.compiler.Get();
    }

    const AssetTypeRegistry::ImporterEntry* AssetTypeRegistry::FindImporter(const TypeInfo& info)
    {
        for (const ImporterEntry& entry : m_importers)
        {
            if (entry.importerInfo == info)
                return &entry;
        }
        return nullptr;
    }

    bool AssetTypeRegistry::RegisterImporter(const TypeInfo& info, UniquePtr<AssetImporter> importer)
    {
        const ImporterEntry* existing = FindImporter(info);

        if (existing != nullptr)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Importer type is already registered."),
                HE_KV(type_hash, info.Hash()),
                HE_KV(type_name, info.Name()),
                HE_KV(existing_name, existing->importerInfo.Name()));
            return false;
        }

        ImporterEntry& entry = m_importers.EmplaceBack();
        entry.importerInfo = info;
        entry.importer = Move(importer);

        return true;
    }

    void AssetTypeRegistry::UnregisterImporter(const TypeInfo& info)
    {
        const ImporterEntry* existing = FindImporter(info);

        if (existing != nullptr)
        {
            const ImporterEntry* begin = m_importers.begin();
            const uint32_t index = static_cast<uint32_t>(std::distance(begin, existing));
            m_importers.Erase(index, 1);
        }
    }

    bool AssetTypeRegistry::RegisterAssetType(
        const char* assetTypeName,
        const he::schema::DeclInfo& declInfo,
        const TypeInfo& compilerInfo,
        UniquePtr<AssetCompiler> compiler)
    {
        const AssetTypeId assetTypeId{ assetTypeName };
        const auto pair = m_assetTypes.try_emplace(assetTypeId);
        if (!pair.second)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset type is already registered."),
                HE_KV(asset_type_name, assetTypeName));
            return false;
        }

        AssetTypeEntry& entry = pair.first->second;
        entry.declInfo = &declInfo;
        entry.compilerInfo = compilerInfo;
        entry.compiler = Move(compiler);

        return true;
    }

    void AssetTypeRegistry::UnregisterAssetType(const char* assetTypeName)
    {
        const AssetTypeId assetTypeId{ assetTypeName };
        m_assetTypes.erase(assetTypeId);
    }
}
