// Copyright Chad Engler

#include "he/window/gamepad.h"

namespace he::window
{
    const char* AsString(GamepadAxis v)
    {
        switch (v)
        {
            case GamepadAxis::None: return "None";
            case GamepadAxis::LThumbX: return "LThumbX";
            case GamepadAxis::LThumbY: return "LThumbY";
            case GamepadAxis::RThumbX: return "RThumbX";
            case GamepadAxis::RThumbY: return "RThumbY";
            case GamepadAxis::LTrigger: return "LTrigger";
            case GamepadAxis::RTrigger: return "RTrigger";
        }

        return "<unknown>";
    }

    const char* AsString(GamepadButton v)
    {

        switch (v)
        {
            case GamepadButton::None: return "None";
            case GamepadButton::DPad_Up: return "DPad_Up";
            case GamepadButton::DPad_Down: return "DPad_Down";
            case GamepadButton::DPad_Left: return "DPad_Left";
            case GamepadButton::DPad_Right: return "DPad_Right";
            case GamepadButton::Start: return "Start";
            case GamepadButton::Back: return "Back";
            case GamepadButton::LThumb: return "LThumb";
            case GamepadButton::RThumb: return "RThumb";
            case GamepadButton::LShoulder: return "LShoulder";
            case GamepadButton::RShoulder: return "RShoulder";
            case GamepadButton::Action1: return "Action1";
            case GamepadButton::Action2: return "Action2";
            case GamepadButton::Action3: return "Action3";
            case GamepadButton::Action4: return "Action4";
        }

        return "<unknown>";
    }
}
