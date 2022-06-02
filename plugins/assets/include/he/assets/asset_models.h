// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/core/uuid.h"

namespace he::assets
{
    class AssetDatabase;

    struct FileProperties
    {
        String path{};
        SystemTime writeTime{ 0 };
        uint32_t fileSize{ 0 };
    };

    struct AssetFileModel final : FileProperties
    {
        Uuid id{};
        FileProperties source{};
        uint32_t scanToken{ 0 };

        static bool AddOrUpdate(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model);
        static bool FindOne(AssetDatabase& db, const Uuid& fileId, AssetFileModel& outModel);
        static bool FindOne(AssetDatabase& db, const char* path, AssetFileModel& outModel);
        static bool RemoveOne(AssetDatabase& db, const Uuid& fileId);
        static bool RemoveAll(AssetDatabase& db, uint32_t scanToken);

        static bool UpdateScanToken(AssetDatabase& db, const Uuid& fileId, uint32_t scanToken);
    };

    struct AssetModel final
    {
        Uuid id;
        Uuid fileId;
        String type;
        String name;
        AssetState state{ AssetState::Unknown };
        uint32_t dataHash{ 0 };
        uint32_t importDataHash{ 0 };
        uint32_t importerId{ 0 };
        uint32_t importerVersion{ 0 };
        uint32_t compilerId{ 0 };
        uint32_t compilerVersion{ 0 };

        static bool AddOrUpdate(AssetDatabase& db, const AssetModel& model);
        static bool AddOrUpdate(AssetDatabase& db, const Uuid& fileId, Asset::Reader asset);
        static bool FindOne(AssetDatabase& db, const Uuid& assetId, AssetModel& model);
        static bool FindAll(AssetDatabase& db, const Uuid& fileId, Vector<AssetModel>& models);
        static bool RemoveOne(AssetDatabase& db, const Uuid& assetId);
    };

    struct ConfigModel final
    {
        String key;
        Vector<uint8_t> value;

        static bool AddOrUpdate(AssetDatabase& db, const ConfigModel& model);
        static bool FindOne(AssetDatabase& db, const char* key, ConfigModel& outModel);
    };

    struct TagModel final
    {
        String tag;
    };

    struct AssetReferenceModel final
    {
        Uuid fromAssetId;
        Uuid toAssetId;

        static bool Add(AssetDatabase& db, const AssetReferenceModel& model);
        static bool FindAllFrom(AssetDatabase& db, const Uuid& fromAssetId);
        static bool FindAllTo(AssetDatabase& db, const Uuid& toAssetId);
        static bool RemoveAllFrom(AssetDatabase& db, const Uuid& fromAssetId);
    };

    struct MessageModel final
    {
        Uuid assetId;
        String message;
        LogLevel level{ LogLevel::Info };
        SystemTime timestamp{ 0 };
        MessageSource source{ MessageSource::Unknown };

        static bool Add(AssetDatabase& db, const MessageModel& model);
        static bool RemoveAll(AssetDatabase& db, const Uuid& refAssetId);
    };

    struct CompilerReferenceModel final
    {
        Uuid assetId;

        // Type of the reference
        CompilerReferenceType referenceType{ CompilerReferenceType::AssetData };

        // Valid for AssetData, AssetBackingFile, and Resource types.
        Uuid toAssetId;

        // Valid for AssetBackingFile, and FilePath types.
        String filePath;

        // Valid for Resource types.
        uint32_t resourceKeyTypeId{ 0 };

        // Valid for Resource types.
        uint32_t resourceKeyHash{ 0 };

        static bool Add(AssetDatabase& db, const CompilerReferenceModel& model);
        static bool RemoveAll(AssetDatabase& db, const Uuid& assetId);
    };
}
