// Copyright Chad Engler

#pragma once

#include "services/platform_service.h"

#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/vector.h"

#include "imgui.h"

namespace he::editor
{
    bool InputText(const char* label, String& str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* userData = nullptr);

    bool InputOpenFile(const char* label, String& path, PlatformService& service, Span<const FileDialogFilter> filters);
    bool InputSaveFile(const char* label, String& path, PlatformService& service, Span<const FileDialogFilter> filters);

    bool InputOpenFolder(const char* label, String& path, PlatformService& service);
}
