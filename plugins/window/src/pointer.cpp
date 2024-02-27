// Copyright Chad Engler

#include "he/window/pointer.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* EnumTraits<window::PointerKind>::ToString(window::PointerKind x) noexcept
    {
        switch (x)
        {
            case window::PointerKind::Mouse: return "Mouse";
            case window::PointerKind::Pen: return "Pen";
            case window::PointerKind::Touch: return "Touch";
            case window::PointerKind::_Count: return "_Count";
        }

        return "<unknown>";
    }

    template <>
    const char* EnumTraits<window::PointerButton>::ToString(window::PointerButton x) noexcept
    {
        switch (x)
        {
            case window::PointerButton::None: return "None";
            case window::PointerButton::Primary: return "Primary";
            case window::PointerButton::Secondary: return "Secondary";
            case window::PointerButton::Auxiliary: return "Auxiliary";
            case window::PointerButton::Extra1: return "Extra1";
            case window::PointerButton::Extra2: return "Extra2";
            case window::PointerButton::Eraser: return "Eraser";
            case window::PointerButton::_Count: return "_Count";
        }

        return "<unknown>";
    }

    template <>
    const char* EnumTraits<window::PointerCursor>::ToString(window::PointerCursor x) noexcept
    {
        switch (x)
        {
            case window::PointerCursor::None: return "None";
            case window::PointerCursor::Arrow: return "Arrow";
            case window::PointerCursor::Hand: return "Hand";
            case window::PointerCursor::NotAllowed: return "NotAllowed";
            case window::PointerCursor::TextInput: return "TextInput";
            case window::PointerCursor::ResizeAll: return "ResizeAll";
            case window::PointerCursor::ResizeTopLeft: return "ResizeTopLeft";
            case window::PointerCursor::ResizeTopRight: return "ResizeTopRight";
            case window::PointerCursor::ResizeBottomLeft: return "ResizeBottomLeft";
            case window::PointerCursor::ResizeBottomRight: return "ResizeBottomRight";
            case window::PointerCursor::ResizeHorizontal: return "ResizeHorizontal";
            case window::PointerCursor::ResizeVertical: return "ResizeVertical";
            case window::PointerCursor::Wait: return "Wait";
            case window::PointerCursor::_Count: return "_Count";
        }

        return "<unknown>";
    }
}
