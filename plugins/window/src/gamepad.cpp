// Copyright Chad Engler

#include "he/window/gamepad.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/window/application.h"
#include "he/window/event.h"

namespace he::window
{
    float Gamepad::GetAxisValue(GamepadAxis axis) const
    {
        if (axis == GamepadAxis::None)
            return 0.0f;

        const uint32_t index = static_cast<uint32_t>(axis);
        HE_ASSERT(index < HE_LENGTH_OF(m_axes));
        return m_axes[index];
    }

    bool Gamepad::IsButtonDown(GamepadButton button) const
    {
        if (button == GamepadButton::None)
            return false;

        const uint32_t flag = (1 << static_cast<uint32_t>(button));
        return HasFlag(m_buttons, flag);
    }

    void Gamepad::SetConnected(Application& app, bool connected)
    {
        if (m_connected == connected)
            return;

        m_connected = connected;

        if (connected)
        {
            Reset();
            GamepadConnectedEvent ev;
            ev.index = m_index;
            app.OnEvent(ev);
        }
        else
        {
            GamepadDisconnectedEvent ev;
            ev.index = m_index;
            app.OnEvent(ev);
        }
    }

    void Gamepad::Reset()
    {
        m_buttons = 0;
        MemZero(m_axes, sizeof(m_axes));
    }

    void Gamepad::SetAxisValue(Application& app, GamepadAxis axis, float value)
    {
        if (axis == GamepadAxis::None)
            return;

        const uint32_t index = static_cast<uint32_t>(axis);
        HE_ASSERT(index < HE_LENGTH_OF(m_axes));

        if (m_axes[index] == value)
            return;

        m_axes[index] = value;

        GamepadAxisEvent ev;
        ev.index = m_index;
        ev.axis = axis;
        ev.value = value;
        app.OnEvent(ev);
    }

    void Gamepad::SetButtonDown(Application& app, GamepadButton button, bool value)
    {
        if (button == GamepadButton::None)
            return;

        const uint32_t flag = (1 << static_cast<uint32_t>(button));

        if (HasFlag(m_buttons, flag) == value)
            return;

        if (value)
        {
            m_buttons |= flag;

            GamepadButtonDownEvent ev;
            ev.index = m_index;
            ev.button = button;
            app.OnEvent(ev);
        }
        else
        {
            m_buttons &= ~flag;

            GamepadButtonUpEvent ev;
            ev.index = m_index;
            ev.button = button;
            app.OnEvent(ev);
        }
    }
}

namespace he
{
    template <>
    const char* EnumTraits<window::GamepadAxis>::ToString(window::GamepadAxis x) noexcept
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
    const char* EnumTraits<window::GamepadButton>::ToString(window::GamepadButton x) noexcept
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
