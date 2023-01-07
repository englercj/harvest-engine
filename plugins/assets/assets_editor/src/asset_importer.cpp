// Copyright Chad Engler

#include "he/assets/asset_importer.h"

namespace he::assets
{
    schema::Asset::Builder ImportResult::CreateAsset(StringView assetTypeName, StringView name)
    {
        schema::Asset::Builder builder = m_builder.AddStruct<schema::Asset>();
        FillUuidV4(builder.InitUuid());
        builder.InitType(assetTypeName);
        builder.InitName(name);
        m_new.PushBack(builder);
        return builder;
    }

    schema::Asset::Builder ImportResult::UpdateAsset(const AssetUuid& assetUuid)
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

    schema::Asset::Builder ImportResult::UpdateAsset(schema::Asset::Reader asset)
    {
        schema::Asset::Builder builder = m_builder.AddStruct<schema::Asset>();
        builder.Copy(asset);
        m_updated.PushBack(builder);
        return builder;
    }
}
