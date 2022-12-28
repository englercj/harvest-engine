// Copyright Chad Engler

#include "asset_edit_service.h"

#include "he/assets/asset_models.h"
#include "he/assets/types_fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/uuid_fmt.h"

namespace he::editor
{
    struct AssetLoadResult
    {
        assets::AssetFileModel fileModel;
        assets::AssetDatabase::AssetFileBuilder file;
        assets::schema::Asset::Builder asset;
    };

    static AssetLoadResult LoadAsset(assets::AssetDatabase& db, const assets::AssetUuid& assetUuid)
    {
        AssetLoadResult result;

        if (!HE_VERIFY(assets::AssetFileModel::FindOne(db, assetUuid, result.fileModel),
            HE_MSG("Unable to find the asset file associated with the asset."),
            HE_KV(asset_uuid, assetUuid)))
        {
            return result;
        }

        assets::AssetDatabase::LoadResult assetLoad = db.LoadAssetFile(result.fileModel.uuid);
        if (!HE_VERIFY(assetLoad.result,
            HE_MSG("Failed to load asset file."),
            HE_KV(path, result.fileModel.file.path),
            HE_KV(uuid, result.fileModel.uuid),
            HE_KV(asset_file_path, result.fileModel.file.path)))
        {
            return result;
        }

        result.file = Move(assetLoad.builder);

        assets::schema::AssetFile::Builder file = result.file.Root();
        for (assets::schema::Asset::Builder asset : file.GetAssets())
        {
            if (assetUuid == asset.GetUuid())
            {
                result.asset = asset;
                break;
            }
        }

        HE_VERIFY(result.asset.IsValid(),
            HE_MSG("Couldn't find asset in file that DB said it was in."),
            HE_KV(asset_uuid, assetUuid),
            HE_KV(asset_file_uuid, result.fileModel.uuid),
            HE_KV(asset_file_path, result.fileModel.file.path));

        return result;
    }

    AssetEditService::AssetEditService(
        AssetService& assetService) noexcept
        : m_assetService(assetService)
    {}

    SchemaEditContext* AssetEditService::OpenAsset(const assets::AssetUuid& assetUuid)
    {
        CtxEntry* existingEntry = m_contexts.Find(assetUuid);
        if (existingEntry)
        {
            ++existingEntry->refCount;
            return existingEntry->ctx.Get();
        }

        assets::AssetDatabase& db = m_assetService.AssetDB();
        AssetLoadResult result = LoadAsset(db, assetUuid);

        if (!result.asset.IsValid())
            return nullptr;

        CtxEntry& entry = m_contexts.Emplace(assetUuid).entry.value;
        entry.ctx = MakeUnique<SchemaEditContext>(result.asset);
        entry.refCount = 1;

        return entry.ctx.Get();
    }

    bool AssetEditService::SaveAsset(const assets::AssetUuid& assetUuid)
    {
        CtxEntry* entry = m_contexts.Find(assetUuid);
        if (HE_VERIFY(entry,
            HE_MSG("SaveAsset called with an asset uuid that is not open")))
        {
            return;
        }

        assets::AssetDatabase& db = m_assetService.AssetDB();
        AssetLoadResult result = LoadAsset(db, assetUuid);

        if (!result.asset.IsValid())
            return;

        result.asset.Copy(entry->ctx->Data().Struct());

        return db.SaveAssetFile(result.fileModel.file.path.Data(), result.file.Root());
    }

    void AssetEditService::CloseAsset(SchemaEditContext* ctx)
    {
        const assets::schema::Asset::Builder& asset = ctx->Data().As<assets::schema::Asset>();
        if (HE_VERIFY(asset.IsValid(),
            HE_MSG("Schema context is not editing. Was it opened with the asset edit service?")))
        {
            return;
        }

        const he::schema::Uuid::Builder& assetUuid = asset.GetUuid();
        CtxEntry* entry = m_contexts.Find(assetUuid);
        if (HE_VERIFY(entry && entry->ctx.Get() == ctx,
            HE_MSG("Schema context not found. Was it opened with the asset edit service?")))
        {
            return;
        }

        if (--entry->refCount == 0)
        {
            m_contexts.Erase(assetUuid);
        }
    }
}
