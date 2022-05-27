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
        SystemTime lastModifiedTime{ 0 };
        uint32_t lastFileSize{ 0 };
    };

    struct AssetFileModel final : FileProperties
    {
        Uuid id;
        uint32_t lastSessionToken{ 0 };
        String path;
        FileProperties source;

        static bool AddOrUpdate(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model);
        static bool FindOne(AssetDatabase& db, const Uuid& fileId, AssetFileModel& model);
        static bool RemoveOne(AssetDatabase& db, const Uuid& fileId);
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

    struct AssetBackingFileModel final : FileProperties
    {
        Uuid assetId;
        String fileName;

        static bool AddOrUpdate(AssetDatabase& db, const AssetBackingFileModel& model);
        static bool FindAll(AssetDatabase& db, const Uuid& assetId, Vector<AssetBackingFileModel>& model);
        static bool RemoveAll(AssetDatabase& db, const Uuid& assetId);
        static bool RemoveOne(AssetDatabase& db, const Uuid& assetId, const char* path);
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
