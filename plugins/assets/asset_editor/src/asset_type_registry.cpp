// Copyright Chad Engler

#include "he/assets/asset_type_registry.h"

#include "he/schema/schema.h"

namespace he::assets
{
    AssetTypeRegistry& AssetTypeRegistry::Get()
    {
        static AssetTypeRegistry s_registry;
        return s_registry;
    }

    bool AssetTypeRegistry::RegisterAssetType(const he::schema::DeclInfo& declInfo, UniquePtr<AssetCompiler> compiler)
    {
        const he::schema::Declaration::Reader decl = he::schema::GetSchema(declInfo);
        if (!decl.IsValid())
            return false;

        const he::schema::Attribute::Reader assetTypeIdAttr = he::schema::FindAttribute(decl.GetAttributes(), schema::AssetType::Id);
        if (!assetTypeIdAttr.IsValid())
            return false;

        const he::schema::Value::Data::Reader assetTypeIdValue = assetTypeIdAttr.GetValue().GetData();
        if (!assetTypeIdValue.IsValid() || !assetTypeIdValue.HasString())
            return false;

        const he::schema::String::Reader assetTypeIdStr = assetTypeIdValue.GetString();
        if (!assetTypeIdStr.IsValid() || assetTypeIdStr.IsEmpty())
            return false;

        AssetTypeId assetTypeId(assetTypeIdStr.Data());

        const auto pair = m_assetTypes.try_emplace(assetTypeId);
        if (!pair.second)
            return false;

        Entry& entry = pair.first->second;
        entry.typeId = assetTypeId;
        entry.declInfo = &declInfo;
        entry.compiler = compiler.Get();

        m_compilers.PushBack(Move(compiler));
        return true;
    }
}
