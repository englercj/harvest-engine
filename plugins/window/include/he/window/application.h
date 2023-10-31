// Copyright Chad Engler

#pragma once

#include "he/math/types.h"

namespace he::window
{
    enum class ViewHitArea : uint8_t;
    enum class ViewDropEffect : uint8_t;
    class View;
    struct Event;

    /// Interface for a windowed application. Handles the application event loop and tick.
    class Application
    {
    public:
        virtual ~Application() = default;

        /// Called during the window event loop to process an event.
        ///
        /// \param[in] ev The event to process.
        virtual void OnEvent(const Event& ev) = 0;

        /// Called each iteration of the event loop, and anytime the OS requests (such as WM_TIMER).
        virtual void OnTick() = 0;

        /// Allows the application to control custom hit testing within the client area. This is
        /// only called when the view is borderless to allow the application to display custom UI
        /// for controling the view, but still allow the OS to handle the behavior of interacting
        /// with that custom UI.
        ///
        /// \param[in] view The view that is being hit test against.
        /// \param[in] point The view-relative point of the hit to test.
        /// \return The area of the hit in the view.
        virtual ViewHitArea OnHitTest(View* view, const Vec2i& point);

        /// Allows the application to control the effect shown a user while performing a
        /// drag-and-drop operation.
        ///
        /// \param view[in] The view the operation is in.
        /// \param point[in] The view-relative point of the pointer.
        /// \return The effect to display for this drag-and-drop operation.
        virtual ViewDropEffect OnDragging(View* view);
    };
}
