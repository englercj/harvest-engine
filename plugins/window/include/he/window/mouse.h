// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he::window
{
    /// Enumeration of the various supported mouse buttons.
    enum class MouseButton : int32_t
    {
        None = -1,      ///< Special value representing no button
        Left = 0,       ///< The left mouse button, sometimes called "mouse 1"
        Right,          ///< The right mouse button, sometimes called "mouse 2"
        Middle,         ///< The middle mouse button, sometimes called "mouse 3"
        Extra1,         ///< An additional mouse button, sometimes called "mouse 4"
        Extra2,         ///< An additional mouse button, sometimes called "mouse 5"
    };

    /// Returns the enum as a string.
    ///
    /// \param[in] v The value to get the string representation of.
    /// \return The string representation of the enum value.
    const char* AsString(MouseButton v);

    /// Enumeration of the various supported cursor display modes.
    enum class MouseCursor : int32_t
    {
        None = -1,          ///< Special value representing no mouse cursor. This hides the cursor from display.
        Arrow = 0,          ///< The default "arrow" cursor
        Hand,               ///< Indicates something can be grabbed.
        NotAllowed,         ///< Indicates that an action is not available.
        TextInput,          ///< Indicates clicking here enabled text input.
        ResizeAll,          ///< Indicates the target can be resized in all directions.
        ResizeTopLeft,      ///< Indicates the target can be grown in the North-West direction.
        ResizeTopRight,     ///< Indicates the target can be grown in the North-East direction.
        ResizeBottomLeft,   ///< Indicates the target can be grown in the South-West direction.
        ResizeBottomRight,  ///< Indicates the target can be grown in the South-East direction.
        ResizeHorizontal,   ///< Indicates the target can be grown in West or East directions.
        ResizeVertical,     ///< Indicates the target can be grown in North or South directions.
        Wait,               ///< Indicates that work is happening and the user should wait.

        _Count,
    };

    /// Returns the enum as a string.
    ///
    /// \param[in] v The value to get the string representation of.
    /// \return The string representation of the enum value.
    const char* AsString(MouseCursor v);
}
