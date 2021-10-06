// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/math/types.h"
#include "he/window/gamepad.h"
#include "he/window/key.h"
#include "he/window/mouse.h"

namespace he::window
{
    class View;

    /// Enumeration of all the types of events that can be passed to \ref Application::OnEvent.
    enum class EventType : uint32_t
    {
        // Mouse events
        MouseDown,              ///< A mouse button has been depressed
        MouseUp,                ///< A mouse button has been released
        MouseWheel,             ///< The mouse wheel has rolled
        MouseMove,              ///< The mouse has moved

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
        ViewDropFile,           ///< A file has been dropped into a view

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
        Event(EventType t)
            : type(t) {}

        /// The type of the event.
        EventType type;
    };

    /// Base structure for an event related to a specific view.
    struct ViewEvent : public Event
    {
        ViewEvent(EventType e, View* v)
            : Event(e), view(v) {}

        /// The target view of the event.
        View* view;
    };

    /// Base structure for MouseUp and MouseDown events.
    struct MouseButtonEvent : public ViewEvent
    {
        MouseButtonEvent(EventType t, View* v, MouseButton b)
            : ViewEvent(t, v), button(b) {}

        /// The button that was pressed or released.
        MouseButton button;
    };

    /// \copydoc EventType::MouseDown
    struct MouseDownEvent : public MouseButtonEvent
    {
        MouseDownEvent(View* v, MouseButton b)
            : MouseButtonEvent(EventType::MouseDown, v, b) {}
    };

    /// \copydoc EventType::MouseUp
    struct MouseUpEvent : public MouseButtonEvent
    {
        MouseUpEvent(View* v, MouseButton b)
            : MouseButtonEvent(EventType::MouseUp, v, b) {}
    };

    /// \copydoc EventType::MouseWheel
    struct MouseWheelEvent : public ViewEvent
    {
        MouseWheelEvent(View* v, const Vec2f& d)
            : ViewEvent(EventType::MouseWheel, v), delta(d) {}

        /// The delta movement of the wheel along the x/y axes.
        Vec2f delta;
    };

    /// \copydoc EventType::MouseMove
    struct MouseMoveEvent : public ViewEvent
    {
        MouseMoveEvent(View* v, const Vec2f& p, bool a, bool r)
            : ViewEvent(EventType::MouseMove, v), pos(p), absolute(a), raw(r) {}

        /// The current position of the mouse.
        Vec2f pos;

        /// Indicates the origin of the `pos` member coordinates. When false `pos` represents
        /// delta movement from the last cursor location. Otherwise it is view-space coordinates.
        bool absolute;

        /// When true the `pos` member is raw input values, i.e. high definition mouse inputs.
        bool raw;
    };

    /// Base structure for KeyUp and KeyDown events.
    struct KeyEvent : public ViewEvent
    {
        KeyEvent(EventType t, View* v, Key k)
            : ViewEvent(t, v), key(k) {}

        /// The key that was pressed or released.
        Key key;
    };

    /// \copydoc EventType::KeyDown
    struct KeyDownEvent : public KeyEvent
    {
        KeyDownEvent(View* v, Key k)
            : KeyEvent(EventType::KeyDown, v, k) {}
    };

    /// \copydoc EventType::KeyUp
    struct KeyUpEvent : public KeyEvent
    {
        KeyUpEvent(View* v, Key k)
            : KeyEvent(EventType::KeyUp, v, k) {}
    };

    /// \copydoc EventType::Text
    struct TextEvent : public ViewEvent
    {
        TextEvent(View* v, char32_t c)
            : ViewEvent(EventType::Text, v), ch(c) {}

        /// The input character.
        char32_t ch;
    };

    /// Base structure for gamepad events.
    struct GamepadEvent : public Event
    {
        GamepadEvent(EventType e, uint32_t i)
            : Event(e), index(i) {}

        /// Index of the gamepad this event is for.
        uint32_t index;
    };

    /// \copydoc EventType::GamepadAxis
    struct GamepadAxisEvent : public GamepadEvent
    {
        GamepadAxisEvent(uint32_t i, GamepadAxis a, float v)
            : GamepadEvent(EventType::GamepadAxis, i), axis(a), value(v) {}

        /// The axis that has changed.
        GamepadAxis axis;

        /// The new normalized value of the axis.
        float value;
    };

    /// \copydoc EventType::GamepadButtonDown
    struct GamepadButtonDownEvent : public GamepadEvent
    {
        GamepadButtonDownEvent(uint32_t i, GamepadButton b)
            : GamepadEvent(EventType::GamepadButtonDown, i), button(b) {}

        /// The button that was pressed.
        GamepadButton button;
    };

    /// \copydoc EventType::GamepadButtonUp
    struct GamepadButtonUpEvent : public GamepadEvent
    {
        GamepadButtonUpEvent(uint32_t i, GamepadButton b)
            : GamepadEvent(EventType::GamepadButtonUp, i), button(b) {}

        /// The button that was released.
        GamepadButton button;
    };

    /// \copydoc EventType::GamepadConnected
    struct GamepadConnectedEvent : public GamepadEvent
    {
        GamepadConnectedEvent(uint32_t i)
            : GamepadEvent(EventType::GamepadConnected, i) {}
    };

    /// \copydoc EventType::GamepadDisconnected
    struct GamepadDisconnectedEvent : public GamepadEvent
    {
        GamepadDisconnectedEvent(uint32_t i)
            : GamepadEvent(EventType::GamepadDisconnected, i) {}
    };

    /// \copydoc EventType::ViewRequestClose
    struct ViewRequestCloseEvent : public ViewEvent
    {
        ViewRequestCloseEvent(View* v)
            : ViewEvent(EventType::ViewRequestClose, v) {}
    };

    /// \copydoc EventType::ViewMoved
    struct ViewMovedEvent : public ViewEvent
    {
        ViewMovedEvent(View* v, const Vec2i& p)
            : ViewEvent(EventType::ViewMoved, v), pos(p) {}

        /// The new position of the view.
        Vec2i pos;
    };

    /// \copydoc EventType::ViewResized
    struct ViewResizedEvent : public ViewEvent
    {
        ViewResizedEvent(View* v, const Vec2i& s)
            : ViewEvent(EventType::ViewResized, v), size(s) {}

        /// The new size of the view.
        Vec2i size;
    };

    /// \copydoc EventType::ViewActivated
    struct ViewActivatedEvent : public ViewEvent
    {
        ViewActivatedEvent(View* v, bool a)
            : ViewEvent(EventType::ViewActivated, v), active(a) {}

        /// True if the view was activated, false if it was deactivated.
        bool active;
    };

    /// \copydoc EventType::ViewDpiScaleChanged
    struct ViewDpiScaleChangedEvent : public ViewEvent
    {
        ViewDpiScaleChangedEvent(View* v, float s)
            : ViewEvent(EventType::ViewDpiScaleChanged, v), scale(s) {}

        /// The new DPI scale of the view.
        float scale;
    };

    /// \copydoc EventType::ViewDropFile
    struct ViewDropFileEvent : public ViewEvent
    {
        ViewDropFileEvent(View* v, const char* p)
            : ViewEvent(EventType::ViewDropFile, v), filePath(p) {}

        /// The file path that was dropped.
        String filePath;
    };

    /// \copydoc EventType::Initialized
    struct InitializedEvent : public ViewEvent
    {
        InitializedEvent(View* v)
            : ViewEvent(EventType::Initialized, v) {}
    };

    /// \copydoc EventType::Terminating
    struct TerminatingEvent : public Event
    {
        TerminatingEvent()
            : Event(EventType::Terminating) {}
    };

    /// \copydoc EventType::Suspending
    struct SuspendingEvent : public Event
    {
        SuspendingEvent()
            : Event(EventType::Suspending) {}
    };

    /// \copydoc EventType::Resuming
    struct ResumingEvent : public Event
    {
        ResumingEvent()
            : Event(EventType::Resuming) {}
    };

    /// \copydoc EventType::DisplayChanged
    struct DisplayChangedEvent : public Event
    {
        DisplayChangedEvent()
            : Event(EventType::DisplayChanged) {}
    };
}
