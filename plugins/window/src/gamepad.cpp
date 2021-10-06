// Copyright Chad Engler

#include "he/window/gamepad.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(window::GamepadAxis x)
    {
        switch (x)
        {
            case window::GamepadAxis::None: return "None";
            case window::GamepadAxis::LThumbX: return "LThumbX";
            case window::GamepadAxis::LThumbY: return "LThumbY";
            case window::GamepadAxis::RThumbX: return "RThumbX";
            case window::GamepadAxis::RThumbY: return "RThumbY";
            case window::GamepadAxis::LTrigger: return "LTrigger";
            case window::GamepadAxis::RTrigger: return "RTrigger";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(window::GamepadButton x)
    {
        switch (x)
        {
            case window::GamepadButton::None: return "None";
            case window::GamepadButton::DPad_Up: return "DPad_Up";
            case window::GamepadButton::DPad_Down: return "DPad_Down";
            case window::GamepadButton::DPad_Left: return "DPad_Left";
            case window::GamepadButton::DPad_Right: return "DPad_Right";
            case window::GamepadButton::Start: return "Start";
            case window::GamepadButton::Back: return "Back";
            case window::GamepadButton::LThumb: return "LThumb";
            case window::GamepadButton::RThumb: return "RThumb";
            case window::GamepadButton::LShoulder: return "LShoulder";
            case window::GamepadButton::RShoulder: return "RShoulder";
            case window::GamepadButton::Action1: return "Action1";
            case window::GamepadButton::Action2: return "Action2";
            case window::GamepadButton::Action3: return "Action3";
            case window::GamepadButton::Action4: return "Action4";
        }

        return "<unknown>";
    }
}
