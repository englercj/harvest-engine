// Copyright Chad Engler

#include "he/window/event.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(window::EventType x)
    {
        switch (x)
        {
            case window::EventType::MouseDown: return  "MouseDown";
            case window::EventType::MouseUp: return  "MouseUp";
            case window::EventType::MouseWheel: return  "MouseWheel";
            case window::EventType::MouseMove: return  "MouseMove";
            case window::EventType::KeyDown: return  "KeyDown";
            case window::EventType::KeyUp: return  "KeyUp";
            case window::EventType::Text: return  "Text";
            case window::EventType::GamepadAxis: return "GamepadAxis";
            case window::EventType::GamepadButtonDown: return "GamepadButtonDown";
            case window::EventType::GamepadButtonUp: return "GamepadButtonUp";
            case window::EventType::GamepadConnected: return "GamepadConnected";
            case window::EventType::GamepadDisconnected: return "GamepadDisconnected";
            case window::EventType::ViewRequestClose: return  "ViewRequestClose";
            case window::EventType::ViewMoved: return  "ViewMoved";
            case window::EventType::ViewResized: return  "ViewResized";
            case window::EventType::ViewActivated: return  "ViewActivated";
            case window::EventType::ViewDpiScaleChanged: return  "ViewDpiScaleChanged";
            case window::EventType::ViewDropFile: return  "ViewDropFile";
            case window::EventType::Initialized: return  "Initialized";
            case window::EventType::Terminating: return  "Terminating";
            case window::EventType::Suspending: return  "Suspending";
            case window::EventType::Resuming: return  "Resuming";
            case window::EventType::DisplayChanged: return  "DisplayChanged";
        }

        return "<unknown>";
    }
}
