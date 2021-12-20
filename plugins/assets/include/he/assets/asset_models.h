// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/vector.h"

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
        AssetFileId id;
        uint32_t lastSessionToken{ 0 };
        String path;
        FileProperties source;

        static bool AddOrUpdate(AssetDatabase& db, AssetFile::Reader file, const AssetFileModel& model);
        static bool FindOne(AssetDatabase& db, const AssetFileId& id, AssetFileModel& model);
        static bool RemoveOne(AssetDatabase& db, const AssetFileId& id);
    };

    struct AssetModel final
    {
        AssetId id;
        AssetFileId fileId;
        String type;
        String name;
        AssetState state{ AssetState::UNKNOWN };
        uint32_t dataHash{ 0 };
        uint32_t importDataHash{ 0 };
        uint32_t importerId{ 0 };
        uint32_t importerVersion{ 0 };
        uint32_t compilerId{ 0 };
        uint32_t compilerVersion{ 0 };

        static bool AddOrUpdate(AssetDatabase& db, const AssetModel& model);
        static bool AddOrUpdate(AssetDatabase& db, const AssetFileId& fileId, Asset::Reader asset);
        static bool FindOne(AssetDatabase& db, const AssetId& id, AssetModel& model);
        static bool FindAll(AssetDatabase& db, const AssetFileId& fileId, Vector<AssetModel>& models);
        static bool RemoveOne(AssetDatabase& db, const AssetId& id);
    };

    struct AssetBackingFileModel final : FileProperties
    {
        AssetId assetId;
        String fileName;

        static bool AddOrUpdate(AssetDatabase& db, const AssetBackingFileModel& model);
        static bool FindAll(AssetDatabase& db, const AssetId& id, Vector<AssetBackingFileModel>& model);
        static bool RemoveAll(AssetDatabase& db, const AssetId& id);
        static bool RemoveOne(AssetDatabase& db, const AssetId& id, const char* path);
    };

    struct AssetReferenceModel final
    {
        AssetId fromId;
        AssetId toId;

        static bool Add(AssetDatabase& db, const AssetReferenceModel& model);
        static bool FindAllFrom(AssetDatabase& db, const AssetId& fromId);
        static bool FindAllTo(AssetDatabase& db, const AssetId& toId);
        static bool RemoveAllFrom(AssetDatabase& db, const AssetId& fromId);
    };

    struct MessageModel final
    {
        AssetId refId;
        String message;
        LogLevel level{ LogLevel::Info };
        SystemTime timestamp{ 0 };
        MessageSource source{ MessageSource::UNKNOWN };

        static bool Add(AssetDatabase& db, const MessageModel& model);
        static bool RemoveAll(AssetDatabase& db, const AssetId& refId);
    };

    struct CompilerReferenceModel final
    {
        AssetId assetId;

        // Type of the reference
        CompilerReferenceType referenceType{ CompilerReferenceType::AssetData };

        // Valid for AssetData, AssetBackingFile, and Resource types.
        AssetId toAssetId;

        // Valid for AssetBackingFile, and FilePath types.
        String filePath;

        // Valid for Resource types.
        uint32_t resourceKeyTypeId{ 0 };

        // Valid for Resource types.
        uint32_t resourceKeyHash{ 0 };

        static bool Add(AssetDatabase& db, const CompilerReferenceModel& model);
        static bool RemoveAll(AssetDatabase& db, const AssetId& id);
    };
}
