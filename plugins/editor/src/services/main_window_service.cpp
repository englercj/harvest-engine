// Copyright Chad Engler

#include "main_window_service.h"

#include "he/math/vec2.h"
#include "he/window/device.h"

namespace he::editor
{
    MainWindowService::MainWindowService(EditorData& editorData) noexcept
        : m_editorData(editorData)
    {}

    void MainWindowService::Quit(int32_t rc)
    {
        m_editorData.device->Quit(rc);
    }

    void MainWindowService::Move(const Vec2i& delta)
    {
        HE_ASSERT(m_view);

        Vec2i p = m_view->GetPosition();
        m_view->SetPosition(p + delta);
    }

}
