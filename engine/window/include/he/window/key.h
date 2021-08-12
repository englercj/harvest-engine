// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he::window
{
    /// Enumeration of the various supported keyboard keys.
    enum class Key : uint32_t
    {
        None,               ///< Special value representing no key

        // Special Keys
        Backspace,
        Enter,
        Escape,
        Space,
        Tab,
        Pause,
        PrintScreen,

        // Numpad
        NumPad_Decimal,
        NumPad_Multiply,
        NumPad_Add,
        NumPad_Subtract,
        NumPad_Divide,
        NumPad_0,
        NumPad_1,
        NumPad_2,
        NumPad_3,
        NumPad_4,
        NumPad_5,
        NumPad_6,
        NumPad_7,
        NumPad_8,
        NumPad_9,

        // Function Keys
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        // Arrow and Directional Keys
        Home,
        Left,
        Up,
        Right,
        Down,
        PageUp,
        PageDown,
        Insert,
        Delete,
        End,

        // Modifer Keys
        Alt,
        Control,
        Shift,
        Super,
        ScrollLock,
        NumLock,
        CapsLock,

        // Number Keys
        Number_0,
        Number_1,
        Number_2,
        Number_3,
        Number_4,
        Number_5,
        Number_6,
        Number_7,
        Number_8,
        Number_9,

        // Text Keys
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        // Puncuation Keys
        Semicolon,
        Equals,
        Comma,
        Minus,
        Period,
        Slash,
        Grave,
        LeftBracket,
        Backslash,
        RightBracket,
        Apostrophe,
    };

    /// Returns the enum as a string.
    ///
    /// \param[in] v The value to get the string representation of.
    /// \return The string representation of the enum value.
    const char* AsString(Key v);
}
