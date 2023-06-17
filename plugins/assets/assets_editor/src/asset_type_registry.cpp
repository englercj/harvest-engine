// Copyright Chad Engler

#include "he/assets/asset_type_registry.h"

#include "he/schema/schema.h"
#include "he/core/name.h"
#include "he/core/name_fmt.h"

namespace he::assets
{
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
            const uint32_t index = static_cast<uint32_t>(existing - begin);
            m_importers.Erase(index, 1);
        }
    }

    bool AssetTypeRegistry::RegisterAssetType(
        Name assetTypeName,
        const schema::DeclInfo& declInfo,
        const TypeInfo& compilerInfo,
        UniquePtr<AssetCompiler> compiler)
    {
        const auto result = m_assetTypes.Emplace(assetTypeName);
        if (!result.inserted)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset type is already registered."),
                HE_KV(asset_type_name, assetTypeName));
            return false;
        }

        const schema::Declaration::Reader decl = GetSchema(declInfo);
        const schema::List<schema::Attribute>::Reader attributes = decl.GetAttributes();

        Entry& entry = result.entry.value;
        entry.declInfo = &declInfo;
        entry.compilerInfo = compilerInfo;
        entry.compiler = Move(compiler);
        entry.importOnly = HasAttribute<editor::Display::ImportOnly>(attributes);

        return true;
    }

    void AssetTypeRegistry::UnregisterAssetType(Name assetTypeName)
    {
        m_assetTypes.Erase(assetTypeName);
    }
}
