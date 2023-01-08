// Copyright Chad Engler

#pragma once

#include "he/editor/services/directory_service.h"
#include "he/editor/schema/settings.hsc.h"
#include "he/schema/layout.h"

namespace he::editor
{
    class SettingsService
    {
    public:
        SettingsService(DirectoryService& directoryService) noexcept;

        void Reset();
        bool Reload();
        bool Save();

        Settings::Builder& GetSettings() { return m_settings; }

    private:
        DirectoryService& m_directoryService;

        schema::Builder m_builder{};
        Settings::Builder m_settings{};
    };
}
