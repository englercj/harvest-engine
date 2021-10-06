// Copyright Chad Engler

#include "settings_service.h"

#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/schema/json.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

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
            HE_LOG_ERROR(editor, HE_MSG("Failed to open project file for reading."),
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
            HE_LOG_ERROR(editor, HE_MSG("Failed to read project file."),
                HE_KV(path, path),
                HE_KV(error, r));
            return false;
        }

        if (bytesRead != fileBuf.Size())
        {
            HE_LOG_ERROR(editor, HE_MSG("Got a short read from project file."),
                HE_KV(path, path),
                HE_KV(expected_bytes, fileBuf.Size()),
                HE_KV(actual_bytes, bytesRead));
            return false;
        }

        file.Close();

        rapidjson::Document doc;
        doc.ParseInsitu(fileBuf.Data());

        if (doc.HasParseError())
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to parse settings file. The JSON document is invalid."),
                HE_KV(path, path),
                HE_KV(error, rapidjson::GetParseError_En(doc.GetParseError())));
            return false;
        }

        if (!he::schema::FromJson(doc, m_settings))
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to parse settings file. The JSON doesn't match the Settings schema."),
                HE_KV(path, path));
            return false;
        }

        return true;
    }

    bool SettingsService::Save()
    {
        String path = m_directoryService.GetAppDirectory(DirectoryService::DirType::Settings);
        ConcatPath(path, SettingsFileName);

        rapidjson::Document doc = he::schema::ToJson(m_settings);

        rapidjson::StringBuffer buf(0, 4096);
        rapidjson::PrettyWriter writer(buf);
        doc.Accept(writer);

        File file;

        Result r = file.Open(path.Data(), FileOpenMode::WriteTruncate);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to open settings file for writing."),
                HE_KV(path, path),
                HE_KV(error, r));
            return false;
        }

        r = file.Write(buf.GetString(), static_cast<uint32_t>(buf.GetSize()));
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
