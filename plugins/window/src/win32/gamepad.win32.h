// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/window/gamepad.h"

#if defined(HE_PLATFORM_API_WIN32)

namespace he::window::win32
{
    class DeviceImpl;

    class GamepadImpl final : public Gamepad
    {
    public:
        GamepadImpl(DeviceImpl* device, const uint32_t index) noexcept
            : Gamepad(index)
            , m_device(device)
        {}

        Result SetVibration(float leftMotorSpeed, float rightMotorSpeed) override;

        void Update(bool refreshConnectivity);

    private:
        DeviceImpl* m_device{ nullptr };
    };
}

#endif
