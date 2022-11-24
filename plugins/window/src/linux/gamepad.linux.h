// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/window/gamepad.h"

#if defined(HE_PLATFORM_LINUX)

struct js_event;

namespace he::window::linux
{
    class DeviceImpl;

    class GamepadImpl final : public Gamepad
    {
    public:
        static constexpr int16_t StickDeadZone = 8000;
        static constexpr int16_t TriggerDeadZone = 8000;

    public:
        GamepadImpl(DeviceImpl* device, const uint32_t index) noexcept
            : Gamepad(index)
            , m_device(device)
        {}

        ~GamepadImpl() noexcept;

        Result SetVibration(float leftMotorSpeed, float rightMotorSpeed) override;

    public:
        void Open();
        void Close();

        void Update(bool refreshConnectivity);
        void UpdateAxis(const js_event& js);
        void UpdateButton(const js_event& js);

    private:
        DeviceImpl* m_device{ nullptr };
        int32_t m_fd{ -1 };
    };
}

#endif
