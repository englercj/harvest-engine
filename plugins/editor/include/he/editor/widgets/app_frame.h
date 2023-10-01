// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/window/view.h"

namespace he::editor
{
    class AppFrame
    {
    public:
        void SetView(window::View* view) { m_view = view; }

        window::ViewHitArea GetHitArea(const Vec2i& point) const;
        bool BeginAppMainMenuBar();
        void EndAppMainMenuBar();

    private:
        window::View* m_view{ nullptr };
        window::ViewHitArea m_menuHitArea{ window::ViewHitArea::Normal };
    };
}
