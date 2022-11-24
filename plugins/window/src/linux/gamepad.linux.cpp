// Copyright Chad Engler

#include "gamepad.linux.h"

#include "device.linux.h"

#include "he/core/enum_ops.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/window/application.h"

#if defined(HE_PLATFORM_LINUX)

#include <errno.h>
#include <fcntl.h>
#include <linux/joystick.h>

namespace he::window::linux
{
    GamepadImpl::~GamepadImpl() noexcept
    {
        Close();
    }

    Result GamepadImpl::SetVibration(float leftMotorSpeed, float rightMotorSpeed)
    {
        // TODO
        HE_UNUSED(leftMotorSpeed, rightMotorSpeed);
        return Result::NotSupported;
    }

    void GamepadImpl::Open()
    {
        if (m_fd != -1)
            return;

        static_assert(MaxGamepads < 10, "Only single-digit gamepad counts are currently supported");

        char jspath[] = "/dev/input/js0";

        constexpr uint32_t JsPathIndex = HE_LENGTH_OF(jspath) - 2;

        jspath[JsPathIndex] = '0' + Index();
        m_fd = open(jspath, O_RDONLY | O_NONBLOCK);

        SetConnected(*m_device->m_app, m_fd != -1);
    }

    void GamepadImpl::Close()
    {
        if (m_fd != -1)
        {
            close(m_fd);
            m_fd = -1;
        }
    }

    void GamepadImpl::Update(bool refreshConnectivity)
    {
        if (refreshConnectivity)
            Open();

        if (!IsConnected())
            return;

        js_event jsEvents[32];
        while (true)
        {
            const int32_t bytesRead = read(m_fd, jsEvents, sizeof(jsEvents));
            if (bytesRead == -1)
            {
                // real error, lets close
                if (errno != EAGAIN)
                    Close();
                break;
            }

            const uint32_t count = bytesRead / sizeof(js_event);

            for (uint32_t i = 0; i < count; ++i)
            {
                const struct js_event& js = jsEvents[i];

                if (HasFlag(js.type, JS_EVENT_AXIS))
                    UpdateAxis(js);
                else if (HasFlag(js.type, JS_EVENT_BUTTON))
                    UpdateButton(js);
            }
        }
    }

    void GamepadImpl::UpdateAxis(const js_event& js)
    {
        HE_ASSERT((js.type & ~JS_EVENT_INIT) == JS_EVENT_AXIS);

        // Normalize the axis value
        float value = js.value / 32767.0f; // value is in the [-32767, 32767] range, normalize to [-1, 1]

        GamepadAxis axis = GamepadAxis::None;
        switch (js.number)
        {
            case 0: axis = GamepadAxis::LThumbX; break;
            case 1: axis = GamepadAxis::LThumbY; break;
            case 2: axis = GamepadAxis::LTrigger; break;
            case 3: axis = GamepadAxis::RThumbX; break;
            case 4: axis = GamepadAxis::RThumbY; break;
            case 5: axis = GamepadAxis::RTrigger; break;
            case 6: // dpad x-axis
            {
                break;
            }
            case 7: // dpad y-axis
            {
                break;
            }
        }

        if (axis != GamepadAxis::None)
        {
            SetAxisValue(*m_device->m_app, axis, value);
        }
    }

    void GamepadImpl::UpdateButton(const js_event& js)
    {
        GamepadButton button = GamepadButton::None;
        switch (js.number)
        {
            case 0: button = GamepadButton::Action1; break;
            case 1: button = GamepadButton::Action2; break;
            case 2: button = GamepadButton::Action3; break;
            case 3: button = GamepadButton::Action4; break;
            case 4: button = GamepadButton::LShoulder; break;
            case 5: button = GamepadButton::RShoulder; break;
            case 6: button = GamepadButton::Back; break;
            case 7: button = GamepadButton::Start; break;
            // case 8: button = GamepadButton::Guide; break;
            case 9: button = GamepadButton::LThumb; break;
            case 10: button = GamepadButton::RThumb; break;
            case 11: button = GamepadButton::DPad_Up; break;
            case 12: button = GamepadButton::DPad_Down; break;
            case 13: button = GamepadButton::DPad_Left; break;
            case 14: button = GamepadButton::DPad_Right; break;
        }

        if (button != GamepadButton::None)
        {
            SetButtonDown(*m_device->m_app, button, js.value == 1);
        }
    }
}

#endif
