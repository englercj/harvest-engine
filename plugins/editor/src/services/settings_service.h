// Copyright Chad Engler

#pragma once

#include "directory_service.h"
#include "schema/settings.capnp.h"

#include "capnp/message.h"

namespace he::editor
{
    using Settings = schema::Settings;

    class SettingsService
    {
    public:
        SettingsService(DirectoryService& directoryService);

        bool Reload();
        bool Save();

        Settings::Builder& GetSettings() { return m_settings; }

    private:
        DirectoryService& m_directoryService;

        capnp::MallocMessageBuilder m_builder{};
        Settings::Builder m_settings{ nullptr };
    };
}
