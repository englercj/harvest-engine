// Copyright Chad Engler

#include "he/editor/services/document_service.h"

#include "he/editor/widgets/menu.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    void DocumentService::DestroyClosedDocuments()
    {
        for (uint32_t i = 0; i < m_documents.Size();)
        {
            UniquePtr<Document>& doc = m_documents[i];

            if (doc->IsClosing())
            {
                if (doc.Get() == m_activeDocument)
                {
                    m_activeDocument = nullptr;
                }

                m_documents.Erase(i, 1);
            }
            else
            {
                ++i;
            }
        }

        if (m_activeDocument == nullptr && !m_documents.IsEmpty())
            m_activeDocument = m_documents[0].Get();
    }

    void DocumentService::ShowDocuments()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiID dockId = ImGui::DockSpaceOverViewport(0, viewport, dockFlags);

        for (UniquePtr<Document>& doc : m_documents)
        {
            ImGui::SetNextWindowDockID(dockId, ImGuiCond_FirstUseEver);
            ImGuiWindowFlags windowFlags = doc->IsDirty() ? ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None;

            const char* label = doc->Label();

            bool open = true;
            bool visible = ImGui::Begin(label, &open, windowFlags);

            if (visible)
                m_activeDocument = doc.Get();

            if (BeginDockTabContextMenu("ContextMenu"))
            {
                doc->ShowContextMenu();
                EndDockTabContextMenu();
            }

            if (!open)
                doc->Close();

            if (visible)
                doc->Show();

            if (doc->IsClosing())
                ImGui::SetTabItemClosed(label);

            ImGui::End();
        }
    }

    void DocumentService::CloseAll()
    {
        for (UniquePtr<Document>& doc : m_documents)
        {
            doc->Close();
        }
    }
}
