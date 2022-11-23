// Copyright Chad Engler

#include "he/window/device.h"

#include "he/core/allocator.h"

namespace he::window
{
    extern Device* _CreateWindowDevice(Allocator& allocator);

    Device* Device::Create(Allocator& allocator)
    {
        Device* device = _CreateWindowDevice(allocator);
        if (!device->Initialize())
        {
            Device::Destroy(device);
            return nullptr;
        }

        return device;
    }

    void Device::Destroy(Device* device)
    {
        if (device)
        {
            device->GetAllocator().Delete(device);
        }
    }
}
