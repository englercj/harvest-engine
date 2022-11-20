// Copyright Chad Engler

#include "gamepad.linux.h"

#include "he/core/enum_ops.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/window/application.h"

#if defined(HE_PLATFORM_LINUX)

#include <fcntl.h>

namespace he::window::linux
{
    constexpr GamepadAxis _heJoyAxisToGamepadAxis[] =
    {
        GamepadAxis::LThumbX,   // 0
        GamepadAxis::LThumbY,   // 1
        GamepadAxis::LTrigger,  // 2
        GamepadAxis::RThumbX,   // 3
        GamepadAxis::RThumbY,   // 4
        GamepadAxis::RTrigger,  // 5
    };

    constexpr GamepadButton _heJoyButtonToGamepadButton[] =
    {
        GamepadButton::Action1,     // 0
        GamepadButton::Action2,     // 1
        GamepadButton::Action3,     // 2
        GamepadButton::Action4,     // 3
        GamepadButton::LShoulder,   // 4
        GamepadButton::RShoulder,   // 5
        GamepadButton::Back,        // 6
        GamepadButton::Start,       // 7
        GamepadButton::None,         // 8, center button - not supported
        GamepadButton::LThumb,      // 9
        GamepadButton::RThumb,      // 10
    };

    GamepadImpl::~GamepadImpl() noexcept
    {
        if (m_fd != -1)
            close(m_fd);
    }

    Result GamepadImpl::SetVibration(float leftMotorSpeed, float rightMotorSpeed)
    {
        // TODO
        return Result::NotSupported;
    }

    void GamepadImpl::Open()
    {
        if (m_fd != -1)
            return;

        static_assert(MaxGamepads < 10, "Only single-digit gamepad counts are currently supported");

        char jspath[] = "/dev/input/js0";

        constexpr uint32_t JsPathIndex = HE_LENGTH_OF(jspath) - 2;

        jspath[JsPathIndex] = '0' + m_index;
        m_fd = open(jspath, O_RDONLY | O_NONBLOCK);

        SetConnected(*m_device->m_app, m_fd != -1);
    }

    void GamepadImpl::Update()
    {
        if (!IsConnected())
            return;

        Application& app = *m_device->m_app;

        js_event jsEvents[8];
        while (true)
        {
            const int32_t bytesRead = read(m_fd, jsEvents, sizeof(jsEvents));
            if (bytesRead == -1)
                break;

            const uint32_t count = bytesRead / sizeof(js_event);

            for (uint32_t i = 0; i < count; ++i)
            {
                const struct js_event& js = jsEvents[i];

                // don't care if this is an init event, so just mask it off
                const uint8_t type = js.type & ~JS_EVENT_INIT;

                if (HasFlag(type, JS_EVENT_AXIS))
                {
                    const uint8_t axisIndex = js.number;

                    // thumb or trigger axis
                    if (axisIndex < HE_LENGTH_OF(_heJoyAxisToGamepadAxis))
                    {
                        // TODO
                    }
                    // dpad x-axis
                    else if (axisIndex == 6)
                    {
                        // TODO
                    }
                    // dpad y-axis
                    else if (axisIndex == 7)
                    {
                        // TODO
                    }
                }

                if (HasFlag(type, JS_EVENT_BUTTON))
                {
                    const uint8_t buttonIndex = js.number;
                    if (buttonIndex < HE_LENGTH_OF(_heJoyButtonToGamepadButton))
                    {
                        const GamepadButton button = _heJoyButtonToGamepadButton[buttonIndex];
                        SetButtonDown(app, button, js.value == 1);
                    }
                }
            }
        }
    }

    void GamepadImpl::Reset()
    {
        m_buttons = 0;
        MemZero(m_axes, sizeof(m_axes));
    }
}

#endif
