// Copyright Chad Engler

#include "he/window/key.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(window::Key x)
    {
        switch (x)
        {
            case window::Key::None: return "None";
            case window::Key::Backspace: return "Backspace";
            case window::Key::Enter: return "Enter";
            case window::Key::Escape: return "Escape";
            case window::Key::Space: return "Space";
            case window::Key::Tab: return "Tab";
            case window::Key::Pause: return "Pause";
            case window::Key::PrintScreen: return "PrintScreen";
            case window::Key::NumPad_Decimal: return "NumPad_Decimal";
            case window::Key::NumPad_Multiply: return "NumPad_Multiply";
            case window::Key::NumPad_Add: return "NumPad_Add";
            case window::Key::NumPad_Subtract: return "NumPad_Subtract";
            case window::Key::NumPad_Divide: return "NumPad_Divide";
            case window::Key::NumPad_Enter: return "NumPad_Enter";
            case window::Key::NumPad_0: return "NumPad_0";
            case window::Key::NumPad_1: return "NumPad_1";
            case window::Key::NumPad_2: return "NumPad_2";
            case window::Key::NumPad_3: return "NumPad_3";
            case window::Key::NumPad_4: return "NumPad_4";
            case window::Key::NumPad_5: return "NumPad_5";
            case window::Key::NumPad_6: return "NumPad_6";
            case window::Key::NumPad_7: return "NumPad_7";
            case window::Key::NumPad_8: return "NumPad_8";
            case window::Key::NumPad_9: return "NumPad_9";
            case window::Key::F1: return "F1";
            case window::Key::F2: return "F2";
            case window::Key::F3: return "F3";
            case window::Key::F4: return "F4";
            case window::Key::F5: return "F5";
            case window::Key::F6: return "F6";
            case window::Key::F7: return "F7";
            case window::Key::F8: return "F8";
            case window::Key::F9: return "F9";
            case window::Key::F10: return "F10";
            case window::Key::F11: return "F11";
            case window::Key::F12: return "F12";
            case window::Key::Home: return "Home";
            case window::Key::Left: return "Left";
            case window::Key::Up: return "Up";
            case window::Key::Right: return "Right";
            case window::Key::Down: return "Down";
            case window::Key::PageUp: return "PageUp";
            case window::Key::PageDown: return "PageDown";
            case window::Key::Insert: return "Insert";
            case window::Key::Delete: return "Delete";
            case window::Key::End: return "End";
            case window::Key::Alt: return "Alt";
            case window::Key::Control: return "Control";
            case window::Key::Shift: return "Shift";
            case window::Key::Super: return "Super";
            case window::Key::ScrollLock: return "ScrollLock";
            case window::Key::NumLock: return "NumLock";
            case window::Key::CapsLock: return "CapsLock";
            case window::Key::Number_0: return "Number_0";
            case window::Key::Number_1: return "Number_1";
            case window::Key::Number_2: return "Number_2";
            case window::Key::Number_3: return "Number_3";
            case window::Key::Number_4: return "Number_4";
            case window::Key::Number_5: return "Number_5";
            case window::Key::Number_6: return "Number_6";
            case window::Key::Number_7: return "Number_7";
            case window::Key::Number_8: return "Number_8";
            case window::Key::Number_9: return "Number_9";
            case window::Key::A: return "A";
            case window::Key::B: return "B";
            case window::Key::C: return "C";
            case window::Key::D: return "D";
            case window::Key::E: return "E";
            case window::Key::F: return "F";
            case window::Key::G: return "G";
            case window::Key::H: return "H";
            case window::Key::I: return "I";
            case window::Key::J: return "J";
            case window::Key::K: return "K";
            case window::Key::L: return "L";
            case window::Key::M: return "M";
            case window::Key::N: return "N";
            case window::Key::O: return "O";
            case window::Key::P: return "P";
            case window::Key::Q: return "Q";
            case window::Key::R: return "R";
            case window::Key::S: return "S";
            case window::Key::T: return "T";
            case window::Key::U: return "U";
            case window::Key::V: return "V";
            case window::Key::W: return "W";
            case window::Key::X: return "X";
            case window::Key::Y: return "Y";
            case window::Key::Z: return "Z";
            case window::Key::Semicolon: return "Semicolon";
            case window::Key::Equals: return "Equals";
            case window::Key::Comma: return "Comma";
            case window::Key::Minus: return "Minus";
            case window::Key::Period: return "Period";
            case window::Key::Slash: return "Slash";
            case window::Key::Grave: return "Grave";
            case window::Key::LeftBracket: return "LeftBracket";
            case window::Key::Backslash: return "Backslash";
            case window::Key::RightBracket: return "RightBracket";
            case window::Key::Apostrophe: return "Apostrophe";
        }

        return "<unknown>";
    }
}
