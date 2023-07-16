// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/hash_table.h"
#include "he/core/name.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/editor/documents/asset_document.h"

namespace he::editor
{
    class AssetDocumentService
    {
    public:
        template <typename T, DerivedFrom<AssetDocument> DocType>
        bool RegisterAssetDocument();

        template <typename T>
        void UnregisterAssetDocument();

        template <typename T>
        UniquePtr<AssetDocument> CreateAssetDocument();

    private:
        using Pfn_CreateAssetDocument = UniquePtr<AssetDocument>(*)();

    private:
        bool RegisterAssetDocument(Name assetTypeName, Pfn_CreateAssetDocument createDoc);
        void UnregisterAssetDocument(Name assetTypeName);
        UniquePtr<AssetDocument> CreateAssetDocument(Name assetTypeName);

    private:
        HashMap<Name, Pfn_CreateAssetDocument> m_assetDocs{};
    };

    // --------------------------------------------------------------------------------------------
    // Inline implementations

    template <typename T, DerivedFrom<AssetDocument> DocType>
    inline bool AssetDocumentService::RegisterAssetDocument()
    {
        const auto createFunc = []() -> UniquePtr<AssetDocument> { return DICreateUnique<DocType>(); };
        return RegisterAssetDocument(Name(T::AssetTypeName), createFunc);
    }

    template <typename T>
    inline void AssetDocumentService::UnregisterAssetDocument()
    {
        return UnregisterAssetDocument(Name(T::AssetTypeName));
    }

    template <typename T>
    inline UniquePtr<AssetDocument> AssetDocumentService::CreateAssetDocument()
    {
        return FindAssetDocument(Name(T::AssetTypeName));
    }
}
