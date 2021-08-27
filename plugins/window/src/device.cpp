// Copyright Chad Engler

#include "he/window/device.h"

#include "he/core/allocator.h"

namespace he::window
{
    extern Device* _CreateDevice(Allocator& allocator);

    Device* CreateDevice(Allocator& allocator)
    {
        Device* device = _CreateDevice(allocator);
        if (!device->Initialize())
        {
            allocator.Delete(device);
            return nullptr;
        }

        return device;
    }

    void DestroyDevice(Device* device)
    {
        device->m_allocator.Delete(device);
    }
}
