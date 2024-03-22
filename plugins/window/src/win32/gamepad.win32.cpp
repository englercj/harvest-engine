// Copyright Chad Engler

#include "gamepad.win32.h"

#include "device.win32.h"

#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <Windows.h>
#include <Xinput.h>

namespace he::window::win32
{
    struct XInputButtonMapping { uint32_t flag; GamepadButton button; };
    constexpr XInputButtonMapping _heXInputButtonMappings[] =
    {
        { XINPUT_GAMEPAD_DPAD_UP, GamepadButton::DPad_Up },
        { XINPUT_GAMEPAD_DPAD_DOWN, GamepadButton::DPad_Down },
        { XINPUT_GAMEPAD_DPAD_LEFT, GamepadButton::DPad_Left },
        { XINPUT_GAMEPAD_DPAD_RIGHT, GamepadButton::DPad_Right },
        { XINPUT_GAMEPAD_START, GamepadButton::Start },
        { XINPUT_GAMEPAD_BACK, GamepadButton::Back },
        { XINPUT_GAMEPAD_LEFT_THUMB, GamepadButton::LThumb },
        { XINPUT_GAMEPAD_RIGHT_THUMB, GamepadButton::RThumb },
        { XINPUT_GAMEPAD_LEFT_SHOULDER, GamepadButton::LShoulder },
        { XINPUT_GAMEPAD_RIGHT_SHOULDER, GamepadButton::RShoulder },
        { XINPUT_GAMEPAD_A, GamepadButton::Action1 },
        { XINPUT_GAMEPAD_B, GamepadButton::Action2 },
        { XINPUT_GAMEPAD_X, GamepadButton::Action3 },
        { XINPUT_GAMEPAD_Y, GamepadButton::Action4 },
    };

    static void NormalizeThumbs(float& x, float& y, SHORT deadZone)
    {
        float mag = Sqrt((x * x) + (y * y));

        if (mag > deadZone)
        {
            x /= mag;
            y /= mag;

            mag = (mag - deadZone) / static_cast<float>(32767 - deadZone);

            x *= mag;
            y *= mag;
        }
        else
        {
            x = 0.0f;
            y = 0.0f;
        }
    }

    static float NormalizeTrigger(float t)
    {
        if (t > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        {
            return static_cast<float>(t - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
        }

        return 0.0f;
    }

    Result GamepadImpl::SetVibration(float leftMotorSpeed, float rightMotorSpeed)
    {
        if (!m_device->m_XInputSetState)
            return Result::NotSupported;

        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotorSpeed * 65535);
        vibration.wRightMotorSpeed = static_cast<WORD>(rightMotorSpeed * 65535);
        DWORD r = m_device->m_XInputSetState(Index(), &vibration);
        return Win32Result(r);
    }

    void GamepadImpl::Update(bool refreshConnectivity)
    {
        if (!IsConnected() && !refreshConnectivity)
            return;

        Application& app = *m_device->m_app;

        XINPUT_STATE xstate{};
        DWORD error = m_device->m_XInputGetState(Index(), &xstate);
        const bool connected = error == ERROR_SUCCESS;

        // If we got a real error, skip this one
        if (!connected && error != ERROR_DEVICE_NOT_CONNECTED)
        {
            Result r = Win32Result(error);
            HE_LOG_ERROR(he_window,
                HE_MSG("Failed to read gamepad state, gamepad will be disconnected."),
                HE_KV(index, Index()),
                HE_KV(result, r));

            SetConnected(app, false);
            return;
        }

        // If connected state changed notify the app.
        SetConnected(app, connected);

        if (!connected)
            return;

        // Update the button states
        const WORD buttons = xstate.Gamepad.wButtons;
        for (const XInputButtonMapping& mapping : _heXInputButtonMappings)
        {
            const bool isPressed = HasFlag(buttons, mapping.flag);
            SetButtonDown(app, mapping.button, isPressed);
        }

        // Update the axis states
        float lx = xstate.Gamepad.sThumbLX;
        float ly = xstate.Gamepad.sThumbLY;
        NormalizeThumbs(lx, ly, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        SetAxisValue(app, GamepadAxis::LThumbX, lx);
        SetAxisValue(app, GamepadAxis::LThumbY, ly);

        float rx = xstate.Gamepad.sThumbRX;
        float ry = xstate.Gamepad.sThumbRY;
        NormalizeThumbs(rx, ry, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        SetAxisValue(app, GamepadAxis::RThumbX, rx);
        SetAxisValue(app, GamepadAxis::RThumbY, ry);

        const float lt = NormalizeTrigger(xstate.Gamepad.bLeftTrigger);
        SetAxisValue(app, GamepadAxis::LTrigger, lt);

        const float rt = NormalizeTrigger(xstate.Gamepad.bRightTrigger);
        SetAxisValue(app, GamepadAxis::RTrigger, rt);
    }
}

#endif
