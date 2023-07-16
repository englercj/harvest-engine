// Copyright Chad Engler

#include "he/editor/services/asset_document_service.h"

#include "he/core/name_fmt.h"

namespace he::editor
{
    bool AssetDocumentService::RegisterAssetDocument(Name assetTypeName, Pfn_CreateAssetDocument createDoc)
    {
        const auto result = m_assetDocs.Emplace(assetTypeName, createDoc);
        if (!result.inserted)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset type already has a registered document type."),
                HE_KV(asset_type_name, assetTypeName));
            return false;
        }

        return true;
    }

    void AssetDocumentService::UnregisterAssetDocument(Name assetTypeName)
    {
        m_assetDocs.Erase(assetTypeName);
    }

    UniquePtr<AssetDocument> AssetDocumentService::CreateAssetDocument(Name assetTypeName)
    {
        Pfn_CreateAssetDocument* createDoc = m_assetDocs.Find(assetTypeName);
        return createDoc ? (*createDoc)() : nullptr;
    }
}
