// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/core/uuid.h"

namespace he::assets
{
    class AssetDatabase;

    enum class AssetState : uint8_t
    {
        Unknown = 0,
        NeedsImport = 1,
        NeedsCompile = 2,
        Ready = 3,

        // Failure states
        ImportFailed = 4,
        CompileFailed = 5,
    };

    struct FileProperties
    {
        String path{};
        SystemTime writeTime{ 0 };
        uint32_t size{ 0 };
    };

    struct AssetFileModel final
    {
        AssetFileUuid uuid{};
        FileProperties file{};
        FileProperties source{};

        static bool AddOrUpdate(AssetDatabase& db, schema::AssetFile::Reader file, const AssetFileModel& model);
        static bool FindOne(AssetDatabase& db, const AssetFileUuid& fileUuid, AssetFileModel& outModel);
        static bool FindOne(AssetDatabase& db, const char* path, AssetFileModel& outModel);
        static bool FindOne(AssetDatabase& db, const AssetUuid& assetUuid, AssetFileModel& outModel);
        static bool RemoveOne(AssetDatabase& db, const AssetFileUuid& fileUuid);
        static bool RemoveOne(AssetDatabase& db, const char* path);
        static bool RemoveOutdated(AssetDatabase& db, uint32_t scanToken);

        static bool UpdateScanToken(AssetDatabase& db, const AssetFileUuid& fileUuid, uint32_t scanToken);
        static bool UpdateScanToken(AssetDatabase& db, const char* path, uint32_t scanToken);
    };

    struct AssetModel final
    {
        AssetUuid uuid;
        AssetFileUuid fileUuid;
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
        static bool AddOrUpdate(AssetDatabase& db, const AssetFileUuid& fileUuid, schema::Asset::Reader asset);
        static bool FindOne(AssetDatabase& db, const AssetUuid& assetUuid, AssetModel& model);
        static bool FindAll(AssetDatabase& db, const AssetFileUuid& fileUuid, Vector<AssetModel>& models);
        static bool FindAll(AssetDatabase& db, const char* search, Vector<AssetModel>& models);
        static bool FindAll(AssetDatabase& db, AssetState state, Vector<AssetModel>& models);
        static bool RemoveOne(AssetDatabase& db, const AssetUuid& assetUuid);

        static bool AddTag(AssetDatabase& db, const AssetUuid& assetUuid, const char* tag);
        static bool RemoveTag(AssetDatabase& db, const AssetUuid& assetUuid, const char* tag);
        static bool RemoveAllTags(AssetDatabase& db, const AssetUuid& assetUuid);

        static bool UpdateState(AssetDatabase& db, const AssetUuid& assetUuid, AssetState state);
    };

    struct ConfigModel final
    {
        String key;
        Vector<uint8_t> value;

        template <typename T> requires(std::is_trivially_copyable_v<T>)
        void SetValue(const T& v)
        {
            value.Resize(sizeof(T), DefaultInit);
            MemCopy(value.Data(), &v, sizeof(T));
        }

        static bool AddOrUpdate(AssetDatabase& db, const ConfigModel& model);
        static bool FindOne(AssetDatabase& db, const char* key, ConfigModel& outModel);
    };

    struct TagModel final
    {
        String tag;
    };

    struct AssetReferenceModel final
    {
        AssetUuid fromAssetUuid;
        AssetUuid toAssetUuid;

        static bool Add(AssetDatabase& db, const AssetReferenceModel& model);
        static bool FindAllFrom(AssetDatabase& db, const AssetUuid& fromAssetUuid);
        static bool FindAllTo(AssetDatabase& db, const AssetUuid& toAssetUuid);
        static bool RemoveAllFrom(AssetDatabase& db, const AssetUuid& fromAssetUuid);
    };

    struct MessageModel final
    {
        enum class Source : uint8_t
        {
            Unknown = 0,
            Importer = 1,
            Compiler = 2,
            System = 3,
            User = 4,
        };

        Uuid refUuid;
        String message;
        LogLevel level{ LogLevel::Info };
        SystemTime timestamp{ 0 };
        Source source{ Source::Unknown };

        static bool Add(AssetDatabase& db, const MessageModel& model);
        static bool RemoveAll(AssetDatabase& db, const Uuid& refUuid);
    };

    struct CompilerReferenceModel final
    {
        enum class Kind : uint8_t
        {
            AssetData = 0,
            AssetBackingFile = 1,
            FilePath = 2,
            Resource = 3,
        };

        AssetUuid assetUuid;

        // Type of the reference
        Kind kind{ Kind::AssetData };

        // Valid for AssetData, AssetBackingFile, and Resource types.
        AssetUuid toAssetUuid;

        // Valid for AssetBackingFile, and FilePath types.
        String filePath;

        // Valid for Resource types.
        uint32_t resourceKeyTypeId{ 0 };

        // Valid for Resource types.
        uint32_t resourceKeyHash{ 0 };

        static bool Add(AssetDatabase& db, const CompilerReferenceModel& model);
        static bool RemoveAll(AssetDatabase& db, const AssetUuid& assetUuid);
    };
}
