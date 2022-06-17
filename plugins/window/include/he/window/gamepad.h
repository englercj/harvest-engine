// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/types.h"

namespace he::window
{
    class Application;

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

    /// Interface for controlling and querying a gamepad device.
    class Gamepad
    {
    public:
        /// Gets the normalized value of a gamepad axis. The value is within the range [0, 1]
        /// where zero is no movement (or within the dead-zone), and one is fully extended.
        ///
        /// \param[in] axis The axis to get the value of.
        /// \return The normalized value of the axis.
        float GetAxisValue(GamepadAxis axis) const;

        /// Checks if a button is currently pressed down.
        ///
        /// \param[in] button The button to check for a pressed state.
        /// \return True if the button is currently pressed down, false otherwise.
        bool IsButtonDown(GamepadButton button) const;

        /// Checks if the gamepad is currently connected.
        ///
        /// \return True if the gamepad is currently connected, false otherwise.
        bool IsConnected() const { return m_connected; }

        /// Gets the index of this gamepad.
        ///
        /// \return The index of the gamepad.
        uint32_t Index() const { return m_index; }

        /// Sets the vibration motor speed for the gamepad. The parameter values are expected to
        /// be in the range [0, 1] where zero is no motor use and one is 100 percent motor use.
        ///
        /// \note For xbox controllers the left motor is the low-frequency rumble motor, and the
        /// right motor is the high-frequency rumble motor. The two motors are not the same, and
        /// they create different vibration effects.
        ///
        /// \param[in] leftMotorSpeed Normalized speed of the left motor.
        /// \param[in] rightMotorSpeed Normalized speed of the right motor.
        /// \return The result of the operation.
        virtual Result SetVibration(float leftMotorSpeed, float rightMotorSpeed) = 0;

    protected:
        Gamepad(uint32_t index) : m_index(index) {}

        void Reset();

        void SetConnected(Application& app, bool connected);
        void SetAxisValue(Application& app, GamepadAxis axis, float value);
        void SetButtonDown(Application& app, GamepadButton button, bool value);

    private:
        const uint32_t m_index;

        uint32_t m_buttons{ 0 };
        float m_axes[6]{};
        bool m_connected{ false };
    };
}
