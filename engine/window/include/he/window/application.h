// Copyright Chad Engler

#pragma once

#include "he/math/types.h"
#include "he/window/view.h"

namespace he::window
{
    struct Event;

    /// Interface for a windowed application. Handles the application event loop and tick.
    class Application
    {
    public:
        virtual ~Application() {}

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
        /// \param[in] view The client view that is being hit test against.
        /// \param[in] point The client-relative point of the hit to test.
        /// \return The area of the hit in the view.
        virtual ViewHitArea OnHitTest(View* view, const Vec2i& point);
    };
}
