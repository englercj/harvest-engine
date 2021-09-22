// Copyright Chad Engler

#pragma once

#include "dialog_service.h"
#include "document_service.h"
#include "imgui_service.h"
#include "main_window_service.h"
#include "platform_service.h"

#include "he/window/view.h"

namespace he::editor
{
    class Document;

    class WorkspaceService
    {
    public:
        WorkspaceService(
            DialogService& dialogService,
            DocumentService& documentService,
            ImGuiService& imguiService,
            MainWindowService& mainWindowService,
            PlatformService& platformService);

        void Show();

        window::ViewHitArea GetHitArea(const Vec2i& point) const;

    private:
        void ShowAppMenuBar();
        void ShowAppStatusBar();

    private:
        DialogService& m_dialogService;
        DocumentService& m_documentService;
        ImGuiService& m_imguiService;
        MainWindowService& m_mainWindowService;
        PlatformService& m_platformService;

        window::ViewHitArea m_menuHitArea{ window::ViewHitArea::Normal };
    };
}
