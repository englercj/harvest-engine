// Copyright Chad Engler

#include "he/editor/services/settings_service.h"

#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/types.h"
#include "he/schema/toml.h"

namespace he::editor
{
    constexpr char SettingsFileName[] = "he_editor_settings.toml";

    SettingsService::SettingsService(DirectoryService& directoryService) noexcept
        : m_directoryService(directoryService)
    {
        Reset();
    }

    void SettingsService::Reset()
    {
        m_builder.Clear();
        m_settings = m_builder.AddStruct<Settings>();
        m_builder.SetRoot(m_settings);
    }

    bool SettingsService::Reload()
    {
        String path = m_directoryService.GetAppDirectory(DirectoryService::DirType::Data);
        ConcatPath(path, SettingsFileName);

        String buf;
        Result r = File::ReadAll(buf, path.Data());
        if (!r)
        {
            if (GetFileResult(r) == FileResult::NotFound)
                return true;

            HE_LOG_ERROR(editor, HE_MSG("Failed to read settings file."),
                HE_KV(path, path),
                HE_KV(result, r));
            return false;
        }

        m_settings = {};
        m_builder.Clear();

        if (!schema::FromToml<Settings>(m_builder, buf.Data()))
        {
            Reset();
            HE_LOG_ERROR(editor, HE_MSG("Failed to deserialize settings file, settings will reset to default. Is it valid TOML?"),
                HE_KV(path, path),
                HE_KV(result, r));
            return false;
        }

        m_settings = m_builder.Root().TryGetStruct<Settings>();
        return true;
    }

    bool SettingsService::Save()
    {
        String path = m_directoryService.GetAppDirectory(DirectoryService::DirType::Data);
        ConcatPath(path, SettingsFileName);

        StringBuilder buf;
        if (!schema::ToToml<Settings>(buf, m_settings))
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to serialize settings file. This is likely an editor bug."));
            return false;
        }

        Result r = File::WriteAll(buf.Str().Data(), buf.Size(), path.Data());
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to write settings file."),
                HE_KV(path, path),
                HE_KV(result, r));
            return false;
        }

        return true;
    }
}
