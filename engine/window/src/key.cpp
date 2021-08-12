// Copyright Chad Engler

#include "he/window/key.h"

namespace he::window
{
    const char* AsString(Key v)
    {
        switch (v)
        {
            case Key::None: return "None";
            case Key::Backspace: return "Backspace";
            case Key::Enter: return "Enter";
            case Key::Escape: return "Escape";
            case Key::Space: return "Space";
            case Key::Tab: return "Tab";
            case Key::Pause: return "Pause";
            case Key::PrintScreen: return "PrintScreen";
            case Key::NumPad_Decimal: return "NumPad_Decimal";
            case Key::NumPad_Multiply: return "NumPad_Multiply";
            case Key::NumPad_Add: return "NumPad_Add";
            case Key::NumPad_Subtract: return "NumPad_Subtract";
            case Key::NumPad_Divide: return "NumPad_Divide";
            case Key::NumPad_0: return "NumPad_0";
            case Key::NumPad_1: return "NumPad_1";
            case Key::NumPad_2: return "NumPad_2";
            case Key::NumPad_3: return "NumPad_3";
            case Key::NumPad_4: return "NumPad_4";
            case Key::NumPad_5: return "NumPad_5";
            case Key::NumPad_6: return "NumPad_6";
            case Key::NumPad_7: return "NumPad_7";
            case Key::NumPad_8: return "NumPad_8";
            case Key::NumPad_9: return "NumPad_9";
            case Key::F1: return "F1";
            case Key::F2: return "F2";
            case Key::F3: return "F3";
            case Key::F4: return "F4";
            case Key::F5: return "F5";
            case Key::F6: return "F6";
            case Key::F7: return "F7";
            case Key::F8: return "F8";
            case Key::F9: return "F9";
            case Key::F10: return "F10";
            case Key::F11: return "F11";
            case Key::F12: return "F12";
            case Key::Home: return "Home";
            case Key::Left: return "Left";
            case Key::Up: return "Up";
            case Key::Right: return "Right";
            case Key::Down: return "Down";
            case Key::PageUp: return "PageUp";
            case Key::PageDown: return "PageDown";
            case Key::Insert: return "Insert";
            case Key::Delete: return "Delete";
            case Key::End: return "End";
            case Key::Alt: return "Alt";
            case Key::Control: return "Control";
            case Key::Shift: return "Shift";
            case Key::Super: return "Super";
            case Key::ScrollLock: return "ScrollLock";
            case Key::NumLock: return "NumLock";
            case Key::CapsLock: return "CapsLock";
            case Key::Number_0: return "Number_0";
            case Key::Number_1: return "Number_1";
            case Key::Number_2: return "Number_2";
            case Key::Number_3: return "Number_3";
            case Key::Number_4: return "Number_4";
            case Key::Number_5: return "Number_5";
            case Key::Number_6: return "Number_6";
            case Key::Number_7: return "Number_7";
            case Key::Number_8: return "Number_8";
            case Key::Number_9: return "Number_9";
            case Key::A: return "A";
            case Key::B: return "B";
            case Key::C: return "C";
            case Key::D: return "D";
            case Key::E: return "E";
            case Key::F: return "F";
            case Key::G: return "G";
            case Key::H: return "H";
            case Key::I: return "I";
            case Key::J: return "J";
            case Key::K: return "K";
            case Key::L: return "L";
            case Key::M: return "M";
            case Key::N: return "N";
            case Key::O: return "O";
            case Key::P: return "P";
            case Key::Q: return "Q";
            case Key::R: return "R";
            case Key::S: return "S";
            case Key::T: return "T";
            case Key::U: return "U";
            case Key::V: return "V";
            case Key::W: return "W";
            case Key::X: return "X";
            case Key::Y: return "Y";
            case Key::Z: return "Z";
            case Key::Semicolon: return "Semicolon";
            case Key::Equals: return "Equals";
            case Key::Comma: return "Comma";
            case Key::Minus: return "Minus";
            case Key::Period: return "Period";
            case Key::Slash: return "Slash";
            case Key::Grave: return "Grave";
            case Key::LeftBracket: return "LeftBracket";
            case Key::Backslash: return "Backslash";
            case Key::RightBracket: return "RightBracket";
            case Key::Apostrophe: return "Apostrophe";
        }

        return "<unknown>";
    }
}
