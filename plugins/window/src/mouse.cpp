// Copyright Chad Engler

#include "he/window/mouse.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(window::MouseButton x)
    {
        switch (x)
        {
            case window::MouseButton::None: return "None";
            case window::MouseButton::Left: return "Left";
            case window::MouseButton::Right: return "Right";
            case window::MouseButton::Middle: return "Middle";
            case window::MouseButton::Extra1: return "Extra1";
            case window::MouseButton::Extra2: return "Extra2";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(window::MouseCursor x)
    {
        switch (x)
        {
            case window::MouseCursor::None: return "None";
            case window::MouseCursor::Arrow: return "Arrow";
            case window::MouseCursor::Hand: return "Hand";
            case window::MouseCursor::NotAllowed: return "NotAllowed";
            case window::MouseCursor::TextInput: return "TextInput";
            case window::MouseCursor::ResizeAll: return "ResizeAll";
            case window::MouseCursor::ResizeTopLeft: return "ResizeTopLeft";
            case window::MouseCursor::ResizeTopRight: return "ResizeTopRight";
            case window::MouseCursor::ResizeBottomLeft: return "ResizeBottomLeft";
            case window::MouseCursor::ResizeBottomRight: return "ResizeBottomRight";
            case window::MouseCursor::ResizeHorizontal: return "ResizeHorizontal";
            case window::MouseCursor::ResizeVertical: return "ResizeVertical";
            case window::MouseCursor::Wait: return "Wait";
            case window::MouseCursor::_Count: return "_Count";
        }

        return "<unknown>";
    }
}
