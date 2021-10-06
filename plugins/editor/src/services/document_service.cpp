// Copyright Chad Engler

#include "document_service.h"

#include "widgets/menu.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    void DocumentService::DestroyClosedDocuments()
    {
        for (uint32_t i = 0; i < m_documents.Size();)
        {
            std::unique_ptr<Document>& doc = m_documents[i];

            if (doc->IsCloseRequested())
            {
                if (doc.get() == m_activeDocument)
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
            m_activeDocument = m_documents[0].get();
    }

    void DocumentService::ShowDocuments()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiID dockId = ImGui::DockSpaceOverViewport(viewport, dockFlags);

        for (std::unique_ptr<Document>& doc : m_documents)
        {
            ImGui::SetNextWindowDockID(dockId, ImGuiCond_FirstUseEver);
            ImGuiWindowFlags windowFlags = doc->IsDirty() ? ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None;

            const char* label = doc->GetLabel();

            bool open = true;
            bool visible = ImGui::Begin(label, &open, windowFlags);

            if (visible)
                m_activeDocument = doc.get();

            if (BeginDockTabContextMenu("ContextMenu"))
            {
                doc->ShowContextMenu();
                EndDockTabContextMenu();
            }

            // TODO: Close request confirmation for dirty documents
            if (!open)
                doc->RequestClose();

            if (visible)
                doc->Show();

            if (doc->IsCloseRequested())
                ImGui::SetTabItemClosed(label);

            ImGui::End();
        }
    }
}
