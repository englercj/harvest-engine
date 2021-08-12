// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he::window
{
    /// Maximum number of supported gamepads.
    constexpr uint32_t MaxGamepads = 4;

    /// Enumeration of the various supported gamepad axes.
    enum class GamepadAxis : int32_t
    {
        None = -1,      ///< Special value representing no axis
        LThumbX = 0,    ///< Left thumb stick, along the x-axis
        LThumbY,        ///< Left thumb stick, along the y-axis
        RThumbX,        ///< Right thumb stick, along the x-axis
        RThumbY,        ///< Right thumb stick, along the y-axis
        LTrigger,       ///< Left analog trigger
        RTrigger,       ///< Right analog trigger
    };

    /// Returns the enum as a string.
    ///
    /// \param[in] v The value to get the string representation of.
    /// \return The string representation of the enum value.
    const char* AsString(GamepadAxis v);

    /// Enumeration of the various supported gamepad buttons.
    enum class GamepadButton : int32_t
    {
        None = -1,      ///< Special value representing no button
        DPad_Up = 0,    ///< Up button on the d-pad
        DPad_Down,      ///< Down button on the d-pad
        DPad_Left,      ///< Left button on the d-pad
        DPad_Right,     ///< Right button on the d-pad
        Start,          ///< Start button
        Back,           ///< Back button, usually to the left of the start button
        LThumb,         ///< Left thumb stick being depressed
        RThumb,         ///< Right thumb stick being depressed
        LShoulder,      ///< Left shoulder button, sometimes called the "left bumper"
        RShoulder,      ///< Right shoulder button, sometimes called the "right bumper"
        Action1,        ///< Bottom action button: A on xbox, X on playstation, B on nintendo
        Action2,        ///< Right action button: B on xbox, O on playstation, A on nintendo
        Action3,        ///< Left action button: X on xbox, ◻ on playstation, Y on nintendo
        Action4,        ///< Top action button: Y on xbox, △ on playstation, X on nintendo
    };

    /// Returns the enum as a string.
    ///
    /// \param[in] v The value to get the string representation of.
    /// \return The string representation of the enum value.
    const char* AsString(GamepadButton v);
}
