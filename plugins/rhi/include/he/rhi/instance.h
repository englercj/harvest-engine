// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/result.h"
#include "he/rhi/types.h"

namespace he::rhi
{
    /// Interface for the RHI instance, which represents the system itself. Provides access to
    /// the system's adapters, displays, and can create rendering devices.
    class Instance
    {
    public:
        /// Creates an instance which is entry object to managing the rendering hardware interface.
        ///
        /// \param[in] desc The descriptor for how to create the instance.
        /// \param[out] out A pointer to the newly created instance.
        /// \return The result of the operation.
        static Result Create(const InstanceDesc& desc, Instance*& out);

        /// Destroys an instance that was created with \ref CreateInstance
        ///
        /// \param[in] instance The instance to destroy.
        static void Destroy(Instance* instance);

    public:
        virtual ~Instance() {}

        /// Gets the allocator instance used for all allocations in this instance.
        ///
        /// \return The allocator.
        virtual Allocator& GetAllocator() = 0;

        /// Translates a result structure into an API result.
        ///
        /// \param[in] result The result object to translate.
        /// \return The API result representing the OS result.
        virtual ApiResult GetApiResult(Result result) const = 0;

        /// Get a list of adapaters (video cards) attached to the system.
        ///
        /// \param[out] count The number of adapaters attached to this system.
        /// \param[out] adapters Array of adapters attached to the system, of length `count`.
        virtual void GetAdapters(uint32_t& count, const Adapter*& adapters) = 0;

        /// Get a list of displays (output devices) an adapter supports.
        ///
        /// \param[in] adapter The adapter to get the displays for.
        /// \param[out] count The number of displays for the adapter.
        /// \param[out] adapters Array of displays for the adapter, of length `count`.
        virtual void GetDisplays(const Adapter& adapter, uint32_t& count, const Display*& displays) = 0;

        /// Get a list of display modes a display supports.
        ///
        /// \param[in] display The display to get the display modes for.
        /// \param[out] count The number of display modes for the adapter.
        /// \param[out] adapters Array of display modes for the display, of length `count`.
        virtual void GetDisplayModes(const Display& display, uint32_t& count, const DisplayMode*& modes) = 0;

        /// Get information about an adapter.
        ///
        /// \param[in] adapter The adapter to get the information of.
        /// \return The adapter information structure.
        virtual const AdapterInfo& GetAdapterInfo(const Adapter& adapter) = 0;

        /// Get information about a display.
        ///
        /// \param[in] display The display to get the information of.
        /// \return The display information structure.
        virtual const DisplayInfo& GetDisplayInfo(const Display& display) = 0;

        /// Get information about a display mode.
        ///
        /// \param[in] mode The display mode to get the information of.
        /// \return The display mode information structure.
        virtual const DisplayModeInfo& GetDisplayModeInfo(const DisplayMode& mode) = 0;

        /// Creates a device used to manage GPU resources and execute commands.
        ///
        /// \param[in] desc The descriptor for how to create the device.
        /// \param[out] out A pointer to the newly created device.
        /// \return The result of the operation.
        virtual Result CreateDevice(const DeviceDesc& desc, Device*& out) = 0;

        /// Destroys a device created with \ref CreateDevice.
        ///
        /// \param[in] device The device to destroy.
        virtual void DestroyDevice(Device* device) = 0;

    protected:
        Instance() = default;

        virtual Result Initialize(const InstanceDesc& desc) = 0;
    };
}
