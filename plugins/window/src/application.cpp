// Copyright Chad Engler

#include "he/window/application.h"

#include "he/core/alloca.h"
#include "he/window/view.h"

namespace he::window
{
    ViewHitArea Application::OnHitTest(View* view, const Vec2i& point)
    {
        Vec2i size = view->GetSize();

        if (point.x < 0 || point.x > size.x || point.y < 0 || point.y > size.y)
            return ViewHitArea::NotInView;

        return ViewHitArea::Normal;
    }

    ViewDropEffect Application::OnDragging([[maybe_unused]] View* view)
    {
        return ViewDropEffect::Reject;
    }
}
