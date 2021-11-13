// Copyright Chad Engler

#include "settings_service.h"

#include "schema/kj_file_stream.h"

#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/types.h"

#include "capnp/serialize.h"
#include "capnp/compat/json.h"

namespace he::editor
{
    constexpr char SettingsFileName[] = "he_editor_settings.json";

    SettingsService::SettingsService(DirectoryService& directoryService)
        : m_directoryService(directoryService)
    {}

    bool SettingsService::Reload()
    {
        String path = m_directoryService.GetAppDirectory(DirectoryService::DirType::Settings);
        ConcatPath(path, SettingsFileName);

        File file;

        Result r = file.Open(path.Data(), FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to open settings file for reading."),
                HE_KV(path, path),
                HE_KV(error, r));
            return false;
        }

        String fileBuf(Allocator::GetTemp());
        fileBuf.Resize(static_cast<uint32_t>(file.GetSize()), DefaultInit);

        uint32_t bytesRead = 0;
        r = file.Read(fileBuf.Data(), fileBuf.Size(), &bytesRead);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to read settings file."),
                HE_KV(path, path),
                HE_KV(error, r));
            return false;
        }

        if (bytesRead != fileBuf.Size())
        {
            HE_LOG_ERROR(editor, HE_MSG("Got a short read from settings file."),
                HE_KV(path, path),
                HE_KV(expected_bytes, fileBuf.Size()),
                HE_KV(actual_bytes, bytesRead));
            return false;
        }

        file.Close();

        m_settings = m_builder.getRoot<Settings>();
        capnp::JsonCodec json;
        json.decode({ fileBuf.Data(), fileBuf.Size() }, m_settings);

        return true;
    }

    bool SettingsService::Save()
    {
        String path = m_directoryService.GetAppDirectory(DirectoryService::DirType::Settings);
        ConcatPath(path, SettingsFileName);

        File file;

        Result r = file.Open(path.Data(), FileOpenMode::WriteTruncate);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to open settings file for writing."),
                HE_KV(path, path),
                HE_KV(error, r));
            return false;
        }

        capnp::JsonCodec json;
        kj::String data = json.encode(m_settings);

        r = file.Write(data.cStr(), static_cast<uint32_t>(data.size()));
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to write settings file."),
                HE_KV(path, path),
                HE_KV(error, r));
            return false;
        }

        return true;
    }
}
