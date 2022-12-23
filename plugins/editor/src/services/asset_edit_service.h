// Copyright Chad Engler

#pragma once

#include "asset_service.h"
#include "framework/schema_edit.h"

#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/assets/types.h"

namespace he::editor
{
    class AssetEditService
    {
    public:
        using AssetBuilder = he::schema::TypedBuilder<assets::schema::Asset>;

        struct LoadResult
        {
            Result result{};
            AssetBuilder builder{};
        };

        using LoadDelegate = Delegate<void(LoadResult)>;

    public:
        explicit AssetEditService(AssetService& assetService) noexcept;

        bool OpenAsset(const assets::AssetUuid& assetUuid);
        void SaveAsset(const assets::AssetUuid& assetUuid);
        void CloseAsset(SchemaEditContext* ctx);

    private:
        struct CtxEntry
        {
            UniquePtr<SchemaEditContext> ctx{ nullptr };
            uint32_t refCount{ 0 };
        };

    private:
        AssetService& m_assetService;

        std::unordered_map<assets::AssetUuid, CtxEntry> m_contexts{};
        std::unordered_map<assets::AssetFileUuid, assets::AssetDatabase::AssetFileBuilder> m_openFiles{};
    };
}
