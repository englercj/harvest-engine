// Copyright Chad Engler

#include "he/window/mouse.h"

namespace he::window
{
    const char* AsString(MouseButton v)
    {
        switch (v)
        {
            case MouseButton::None: return "None";
            case MouseButton::Left: return "Left";
            case MouseButton::Right: return "Right";
            case MouseButton::Middle: return "Middle";
            case MouseButton::Extra1: return "Extra1";
            case MouseButton::Extra2: return "Extra2";
        }

        return "<unknown>";
    }

    const char* AsString(MouseCursor v)
    {
        switch (v)
        {
            case MouseCursor::None: return "None";
            case MouseCursor::Arrow: return "Arrow";
            case MouseCursor::Hand: return "Hand";
            case MouseCursor::NotAllowed: return "NotAllowed";
            case MouseCursor::TextInput: return "TextInput";
            case MouseCursor::ResizeAll: return "ResizeAll";
            case MouseCursor::ResizeTopLeft: return "ResizeTopLeft";
            case MouseCursor::ResizeTopRight: return "ResizeTopRight";
            case MouseCursor::ResizeBottomLeft: return "ResizeBottomLeft";
            case MouseCursor::ResizeBottomRight: return "ResizeBottomRight";
            case MouseCursor::ResizeHorizontal: return "ResizeHorizontal";
            case MouseCursor::ResizeVertical: return "ResizeVertical";
            case MouseCursor::Wait: return "Wait";
            case MouseCursor::_Count: return "_Count";
        }

        return "<unknown>";
    }
}
