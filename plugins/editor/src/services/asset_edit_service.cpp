// Copyright Chad Engler

#include "asset_edit_service.h"

#include "he/assets/asset_models.h"
#include "he/assets/types_fmt.h"

namespace he::editor
{
    AssetEditService::AssetEditService(
        AssetService& assetService) noexcept
        : m_assetService(assetService)
    {}

    SchemaEditContext* AssetEditService::OpenAsset(const assets::AssetUuid& assetUuid)
    {
        auto ctxIt = m_contexts.find(assetUuid);
        if (ctxIt != m_contexts.end())
            return ctxIt->second.ctx.Get();

        assets::AssetDatabase& db = m_assetService.AssetDB();

        assets::AssetFileModel assetFile;
        if (!HE_VERIFY(assets::AssetFileModel::FindOne(db, assetUuid, assetFile),
            HE_MSG("Unable to find the asset file associated with the asset."),
            HE_KV(asset_uuid, assetUuid)))
        {
            return nullptr;
        }

        auto fileIt = m_openFiles.find(assetFile.uuid);
        if (fileIt != m_openFiles.end())
        {
            assets::AssetDatabase::AssetFileBuilder& builder = fileIt->second;
            for (assets::schema::Asset::Builder asset : builder.Root().GetAssets())
            {
                if (assetUuid == asset.GetUuid())
                {
                    CtxEntry entry;
                    entry.ctx = MakeUnique<SchemaEditContext>(asset);
                    entry.refCount = 1;
                    auto pair = m_contexts.emplace(assetUuid, Move(entry));
                    return pair.first->second.ctx.Get();
                }
            }

            HE_VERIFY(false,
                HE_MSG("Couldn't find asset in file that DB said it was in."),
                HE_KV(asset_uuid, assetUuid),
                HE_KV(asset_file_uuid, assetFile.uuid));
        }
        else
        {
            // TODO: Load file
        }
    }

    void AssetEditService::SaveAsset(const assets::AssetUuid& assetUuid)
    {
        // TODO: Load file, copy asset data over, save asset file
        assets::AssetDatabase& db = m_assetService.AssetDB();

        assets::AssetFileModel assetFile;
        if (!HE_VERIFY(assets::AssetFileModel::FindOne(db, assetUuid, assetFile),
            HE_MSG("Unable to find the asset file associated with the asset."),
            HE_KV(asset_uuid, assetUuid)))
        {
            return nullptr;
        }
    }

    void AssetEditService::CloseAsset(SchemaEditContext* ctx)
    {
        // TODO: close context
    }
}
