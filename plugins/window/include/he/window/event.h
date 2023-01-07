// Copyright Chad Engler

#pragma once

#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/math/types.h"
#include "he/window/gamepad.h"
#include "he/window/key.h"
#include "he/window/pointer.h"

namespace he::window
{
    class View;

    /// Enumeration of all the kinds of events that can be passed to \ref Application::OnEvent.
    enum class EventKind : uint32_t
    {
        // Mouse events
        PointerDown,            ///< A pointer has become active \see PointerDownEvent
        PointerUp,              ///< A pointer is no longer active state
        PointerMove,            ///< A pointer has changed coordinates
        PointerWheel,           ///< A pointer's wheel button has been rotated, typically a mouse wheel

        // Keyboard events
        KeyDown,                ///< A keyboard key has been depressed
        KeyUp,                  ///< A keyboard key has been released
        Text,                   ///< The text emitted by a keyboard key that was pressed

        // Gamepad events
        GamepadAxis,            ///< A gamepad axis has moved
        GamepadButtonDown,      ///< A gamepad button has been depressed
        GamepadButtonUp,        ///< A gamepad button has been released
        GamepadConnected,       ///< A new gamepad has been connected
        GamepadDisconnected,    ///< A gamepad has been disconnected

        // View events
        ViewRequestClose,       ///< A view is requesting to be closed
        ViewMoved,              ///< A view has been moved
        ViewResized,            ///< A view has been resized
        ViewActivated,          ///< A view has been activated
        ViewDpiScaleChanged,    ///< The DPI scale of a view has changed
        ViewDndStart,           ///< A drag-and-drop operation has started on a view
        ViewDndMove,            ///< The drop target location has changed, this is similar to a PointerMove event
        ViewDndDrop,            ///< An object has been dropped into a view
        ViewDndEnd,             ///< The drag-and-drop operation has completed

        // App events
        Initialized,            ///< The application has been initialized, and the default view was created
        Terminating,            ///< The application is terminating
        Suspending,             ///< The application is being suspended
        Resuming,               ///< The application is being resumed
        DisplayChanged,         ///< A display has changed, usually because of the resolution being changed
    };

    /// Base structure for an event.
    struct Event
    {
        explicit Event(EventKind k) noexcept
            : kind(k) {}

        /// The kind of event this is.
        const EventKind kind;
    };

    /// Base structure for an event related to a specific view.
    struct ViewEvent : Event
    {
        explicit ViewEvent(EventKind k, View* v) noexcept
            : Event(k), view(v) {}

        /// The target view of the event.
        View* view;
    };

    /// Base pointer event structure used for all pointer events.
    struct PointerEvent : ViewEvent
    {
        explicit PointerEvent(EventKind k, View* v) noexcept
            : ViewEvent(k, v) {}

        PointerId pointerId{ 0 };   ///< A unique identifier for the pointer causing the event.
        PointerKind pointerKind{};  ///< The kind of the pointer device.
        Vec2i size{ 1, 1 };         ///< The width(x) & height(y) of the contact geometry of the pointer.
        Vec2i tilt{ 0, 0 };         ///< Plane angle (in degrees, [-90, 90]) between the X–Z plane and the plane containing both the pointer (e.g. pen stylus) axis and the X axis.
        uint32_t rotation{ 0 };     ///< The clockwise rotation of the pointer (e.g. pen stylus) around its major axis in degrees, with a value in the range [0, 359].
        float pressure{ 0 };        ///< Normalized pressure of the pointer, where 0 and 1 represent the minimum and maximum pressure the hardware is capable of detecting, respectively.

        /// Indicates if the pointer represents the primary pointer of this pointer kind.
        ///
        /// For mouse, there is only one pointer, so it will always be the primary pointer.
        /// For touch input, a pointer is considered primary if the user touched the screen when
        /// there were no other active touches.
        /// For pen and stylus input, a pointer is considered primary if the user's pen initially
        /// contacted the screen when there were no other active pens contacting the screen.
        bool isPrimary{ false };
    };

    /// Base structure for PointerDown and PointerUp events.
    struct PointerButtonEvent : PointerEvent
    {
        explicit PointerButtonEvent(EventKind k, View* v) noexcept
            : PointerEvent(k, v) {}

        /// Button that either became active or inactive with this event.
        PointerButton button{ PointerButton::None };
    };

    /// \copydoc EventKind::PointerUp
    ///
    /// A pointer has become active. For mouse, this happens when the device transitions from no
    /// buttons pressed to at least one button pressed. For touch, this happens when physical
    /// contact is made with the digitizer. For pen, this happens when the stylus makes physical
    /// contact with the digitizer.
    struct PointerDownEvent : PointerButtonEvent
    {
        explicit PointerDownEvent(View* v) noexcept
            : PointerButtonEvent(EventKind::PointerDown, v) {}
    };

    /// \copydoc EventKind::PointerUp
    struct PointerUpEvent : PointerButtonEvent
    {
        explicit PointerUpEvent(View* v) noexcept
            : PointerButtonEvent(EventKind::PointerUp, v) {}
    };

    /// \copydoc EventKind::PointerMove
    struct PointerMoveEvent : PointerEvent
    {
        explicit PointerMoveEvent(View* v) noexcept
            : PointerEvent(EventKind::PointerMove, v) {}

        /// When absolute is true this represents the position of the mouse in screen space.
        /// When absolute is false this represents the delta movement of the mouse from the
        /// last mouse position.
        Vec2f pos{ 0, 0 };

        /// Indicates the origin of the `pos` member coordinates. When false `pos` represents
        /// delta movement from the last cursor location. Otherwise it is view-space coordinates.
        bool absolute{ false };
    };

    /// \copydoc EventKind::PointerWheel
    struct PointerWheelEvent : PointerEvent
    {
        explicit PointerWheelEvent(View* v) noexcept
            : PointerEvent(EventKind::PointerWheel, v) {}

        /// The delta movement of the wheel along the x & y axes.
        Vec2f delta{ 0 , 0 };
    };

    /// Base structure for KeyUp and KeyDown events.
    struct KeyEvent : ViewEvent
    {
        explicit KeyEvent(EventKind k, View* v) noexcept
            : ViewEvent(k, v) {}

        /// The key that was pressed or released.
        Key key{ Key::None };
    };

    /// \copydoc EventKind::KeyDown
    struct KeyDownEvent : KeyEvent
    {
        explicit KeyDownEvent(View* v) noexcept
            : KeyEvent(EventKind::KeyDown, v) {}
    };

    /// \copydoc EventKind::KeyUp
    struct KeyUpEvent : KeyEvent
    {
        explicit KeyUpEvent(View* v) noexcept
            : KeyEvent(EventKind::KeyUp, v) {}
    };

    /// \copydoc EventKind::Text
    struct TextEvent : ViewEvent
    {
        explicit TextEvent(View* v) noexcept
            : ViewEvent(EventKind::Text, v) {}

        /// The input character.
        char16_t ch{ 0 };
    };

    /// Base structure for gamepad events.
    struct GamepadEvent : Event
    {
        explicit GamepadEvent(EventKind k) noexcept
            : Event(k) {}

        /// Index of the gamepad this event is for.
        uint32_t index{ 0 };
    };

    /// \copydoc EventKind::GamepadAxis
    struct GamepadAxisEvent : GamepadEvent
    {
        explicit GamepadAxisEvent() noexcept
            : GamepadEvent(EventKind::GamepadAxis) {}

        /// The axis that has changed.
        GamepadAxis axis{ GamepadAxis::None };

        /// The new normalized value of the axis.
        float value{ 0 };
    };

    /// Base structure for GamepadButtonDown and GamepadButtonUp events.
    struct GamepadButtonEvent : GamepadEvent
    {
        explicit GamepadButtonEvent(EventKind k) noexcept
            : GamepadEvent(k) {}

        /// The button that was pressed or released.
        GamepadButton button{ GamepadButton::None };
    };

    /// \copydoc EventKind::GamepadButtonDown
    struct GamepadButtonDownEvent : GamepadButtonEvent
    {
        explicit GamepadButtonDownEvent() noexcept
            : GamepadButtonEvent(EventKind::GamepadButtonDown) {}
    };

    /// \copydoc EventKind::GamepadButtonUp
    struct GamepadButtonUpEvent : GamepadButtonEvent
    {
        explicit GamepadButtonUpEvent() noexcept
            : GamepadButtonEvent(EventKind::GamepadButtonUp) {}
    };

    /// \copydoc EventKind::GamepadConnected
    struct GamepadConnectedEvent : GamepadEvent
    {
        explicit GamepadConnectedEvent() noexcept
            : GamepadEvent(EventKind::GamepadConnected) {}
    };

    /// \copydoc EventKind::GamepadDisconnected
    struct GamepadDisconnectedEvent : GamepadEvent
    {
        explicit GamepadDisconnectedEvent() noexcept
            : GamepadEvent(EventKind::GamepadDisconnected) {}
    };

    /// \copydoc EventKind::ViewRequestClose
    struct ViewRequestCloseEvent : ViewEvent
    {
        explicit ViewRequestCloseEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewRequestClose, v) {}
    };

    /// \copydoc EventKind::ViewMoved
    struct ViewMovedEvent : ViewEvent
    {
        explicit ViewMovedEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewMoved, v) {}

        /// The new position of the view.
        Vec2i pos{ 0, 0 };
    };

    /// \copydoc EventKind::ViewResized
    struct ViewResizedEvent : ViewEvent
    {
        explicit ViewResizedEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewResized, v) {}

        /// The new size of the view.
        Vec2i size{ 0, 0 };
    };

    /// \copydoc EventKind::ViewActivated
    struct ViewActivatedEvent : ViewEvent
    {
        explicit ViewActivatedEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewActivated, v) {}

        /// True if the view was activated, false if it was deactivated.
        bool active{ false };
    };

    /// \copydoc EventKind::ViewDpiScaleChanged
    struct ViewDpiScaleChangedEvent : ViewEvent
    {
        explicit ViewDpiScaleChangedEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewDpiScaleChanged, v) {}

        /// The new DPI scale of the view.
        float scale{ 0 };
    };

    /// \copydoc EventKind::ViewDndStart
    struct ViewDndStartEvent : ViewEvent
    {
        explicit ViewDndStartEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewDndStart, v) {}
    };

    /// \copydoc EventKind::ViewDndMove
    struct ViewDndMoveEvent : ViewEvent
    {
        explicit ViewDndMoveEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewDndMove, v) {}

        /// The absolute position of the cursor when the drop position changes.
        Vec2f pos{ 0, 0 };
    };

    /// \copydoc EventKind::ViewDndDrop
    struct ViewDndDropEvent : ViewEvent
    {
        explicit ViewDndDropEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewDndDrop, v) {}

        /// The path of the dropped object.
        StringView path{};
    };

    /// \copydoc EventKind::ViewDndEnd
    struct ViewDndEndEvent : ViewEvent
    {
        explicit ViewDndEndEvent(View* v) noexcept
            : ViewEvent(EventKind::ViewDndEnd, v) {}
    };

    /// \copydoc EventKind::Initialized
    struct InitializedEvent : ViewEvent
    {
        explicit InitializedEvent(View* v) noexcept
            : ViewEvent(EventKind::Initialized, v) {}
    };

    /// \copydoc EventKind::Terminating
    struct TerminatingEvent : Event
    {
        TerminatingEvent() noexcept
            : Event(EventKind::Terminating) {}
    };

    /// \copydoc EventKind::Suspending
    struct SuspendingEvent : Event
    {
        SuspendingEvent() noexcept
            : Event(EventKind::Suspending) {}
    };

    /// \copydoc EventKind::Resuming
    struct ResumingEvent : Event
    {
        ResumingEvent() noexcept
            : Event(EventKind::Resuming) {}
    };

    /// \copydoc EventKind::DisplayChanged
    struct DisplayChangedEvent : Event
    {
        DisplayChangedEvent() noexcept
            : Event(EventKind::DisplayChanged) {}
    };
}
