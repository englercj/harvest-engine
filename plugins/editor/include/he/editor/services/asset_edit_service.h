// Copyright Chad Engler

#pragma once

#include "he/core/hash_table.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/assets/types.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/framework/schema_edit.h"

namespace he::editor
{
    class AssetEditService
    {
    public:
        using AssetBuilder = schema::TypedBuilder<assets::Asset>;

        struct LoadResult
        {
            Result result{};
            AssetBuilder builder{};
        };

        using LoadDelegate = Delegate<void(LoadResult)>;

    public:
        explicit AssetEditService(AssetService& assetService) noexcept;

        SchemaEditContext* OpenAsset(const assets::AssetUuid& assetUuid);
        bool SaveAsset(const assets::AssetUuid& assetUuid);
        void CloseAsset(SchemaEditContext* ctx);

    private:
        struct CtxEntry
        {
            UniquePtr<SchemaEditContext> ctx{ nullptr };
            uint32_t refCount{ 0 };
        };

    private:
        AssetService& m_assetService;

        HashMap<assets::AssetUuid, CtxEntry> m_contexts{};
    };
}
