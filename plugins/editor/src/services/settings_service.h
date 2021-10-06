// Copyright Chad Engler

#pragma once

#include "directory_service.h"
#include "schema/settings.generated.h"

namespace he::editor
{
    using Settings = schema::Settings;

    class SettingsService
    {
    public:
        SettingsService(DirectoryService& directoryService);

        bool Reload();
        bool Save();

        Settings& GetSettings() { return m_settings; }

    private:
        DirectoryService& m_directoryService;

        Settings m_settings{};
    };
}
