// Copyright Chad Engler

#include "he/window/event.h"

namespace he::window
{
    const char* AsString(EventType v)
    {
        switch (v)
        {
            case EventType::MouseDown: return  "MouseDown";
            case EventType::MouseUp: return  "MouseUp";
            case EventType::MouseWheel: return  "MouseWheel";
            case EventType::MouseMove: return  "MouseMove";
            case EventType::KeyDown: return  "KeyDown";
            case EventType::KeyUp: return  "KeyUp";
            case EventType::Text: return  "Text";
            case EventType::GamepadAxis: return "GamepadAxis";
            case EventType::GamepadButtonDown: return "GamepadButtonDown";
            case EventType::GamepadButtonUp: return "GamepadButtonUp";
            case EventType::GamepadConnected: return "GamepadConnected";
            case EventType::GamepadDisconnected: return "GamepadDisconnected";
            case EventType::ViewRequestClose: return  "ViewRequestClose";
            case EventType::ViewMoved: return  "ViewMoved";
            case EventType::ViewResized: return  "ViewResized";
            case EventType::ViewActivated: return  "ViewActivated";
            case EventType::ViewDpiScaleChanged: return  "ViewDpiScaleChanged";
            case EventType::ViewDropFile: return  "ViewDropFile";
            case EventType::Initialized: return  "Initialized";
            case EventType::Terminating: return  "Terminating";
            case EventType::Suspending: return  "Suspending";
            case EventType::Resuming: return  "Resuming";
            case EventType::DisplayChanged: return  "DisplayChanged";
        }

        return "<unknown>";
    }
}
