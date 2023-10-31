#pragma once

#include "he/window/application.h"
#include "he/window/device.h"

namespace he
{
    class TestApp : public window::Application
    {
    public:
        TestApp(window::Device* device);

        void OnEvent(const window::Event& ev) override;
        void OnTick() override;

    private:
        window::Device* m_device{ nullptr };
        window::View* m_view{ nullptr };
    };
}
