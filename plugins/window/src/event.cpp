// Copyright Chad Engler

#include "he/window/event.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(window::EventKind x)
    {
        switch (x)
        {
            case window::EventKind::PointerDown: return "PointerDown";
            case window::EventKind::PointerUp: return "PointerUp";
            case window::EventKind::PointerWheel: return "PointerWheel";
            case window::EventKind::PointerMove: return "PointerMove";
            case window::EventKind::KeyDown: return "KeyDown";
            case window::EventKind::KeyUp: return "KeyUp";
            case window::EventKind::Text: return "Text";
            case window::EventKind::GamepadAxis: return "GamepadAxis";
            case window::EventKind::GamepadButtonDown: return "GamepadButtonDown";
            case window::EventKind::GamepadButtonUp: return "GamepadButtonUp";
            case window::EventKind::GamepadConnected: return "GamepadConnected";
            case window::EventKind::GamepadDisconnected: return "GamepadDisconnected";
            case window::EventKind::ViewRequestClose: return "ViewRequestClose";
            case window::EventKind::ViewMoved: return "ViewMoved";
            case window::EventKind::ViewResized: return "ViewResized";
            case window::EventKind::ViewActivated: return "ViewActivated";
            case window::EventKind::ViewDpiScaleChanged: return "ViewDpiScaleChanged";
            case window::EventKind::ViewDndStart: return "ViewDndStart";
            case window::EventKind::ViewDndMove: return "ViewDndMove";
            case window::EventKind::ViewDndDrop: return "ViewDndDrop";
            case window::EventKind::ViewDndEnd: return "ViewDndEnd";
            case window::EventKind::Initialized: return "Initialized";
            case window::EventKind::Terminating: return "Terminating";
            case window::EventKind::Suspending: return "Suspending";
            case window::EventKind::Resuming: return "Resuming";
            case window::EventKind::DisplayChanged: return "DisplayChanged";
        }

        return "<unknown>";
    }
}
