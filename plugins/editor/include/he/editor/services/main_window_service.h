// Copyright Chad Engler

#pragma once

#include "he/editor/editor_data.h"
#include "he/math/types.h"
#include "he/window/view.h"

namespace he::editor
{
    class MainWindowService
    {
    public:
        MainWindowService(EditorData& editorData) noexcept;

        void SetView(window::View* view) { m_view = view; }
        window::View* GetView() { return m_view; }

        void Quit(int32_t rc);

        void Move(const Vec2i& delta);

    private:
        EditorData& m_editorData;

        window::View* m_view{ nullptr };
    };
}
