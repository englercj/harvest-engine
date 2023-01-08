// Copyright Chad Engler

#include "he/assets/asset_importer.h"

namespace he::assets
{
    Asset::Builder ImportResult::CreateAsset(StringView assetTypeName, StringView name)
    {
        Asset::Builder builder = m_builder.AddStruct<Asset>();
        FillUuidV4(builder.InitUuid());
        builder.InitType(assetTypeName);
        builder.InitName(name);
        m_new.PushBack(builder);
        return builder;
    }

    Asset::Builder ImportResult::UpdateAsset(const AssetUuid& assetUuid)
    {
        for (auto&& asset : m_ctx.assetFile.GetAssets())
        {
            if (asset.GetUuid() == assetUuid)
            {
                return UpdateAsset(asset);
            }
        }

        return {};
    }

    Asset::Builder ImportResult::UpdateAsset(Asset::Reader asset)
    {
        Asset::Builder builder = m_builder.AddStruct<Asset>();
        builder.Copy(asset);
        m_updated.PushBack(builder);
        return builder;
    }
}
