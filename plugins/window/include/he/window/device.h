// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/span.h"
#include "he/math/types.h"
#include "he/window/gamepad.h"
#include "he/window/pointer.h"
#include "he/window/view.h"

namespace he::window
{
    class Application;
    class View;
    struct ViewDesc;

    /// Description of a display monitor device.
    struct Monitor
    {
        Vec2i pos;      ///< Position of the device in screen space.
        Vec2i size;     ///< Size of the monitor
        Vec2i workPos;  ///< Working position of the monitor (excludes task bars, menu bars, etc)
        Vec2i workSize; ///< Working size of the monitor (excludes task bars, menu bars, etc)
        float dpiScale; ///< Gets the DPI scale of a monitor, where 1.0 is 96 DPI
        bool primary;   ///< True if the monitor is the primary monitor
    };

    /// Information about an initialized device.
    struct DeviceInfo
    {
        bool hasMouse{ false };
        bool hasHighDefMouse{ false };
        bool hasTouch{ false };
        bool hasPen{ false };
        uint32_t maxTouches{ 0 };
    };

    /// Interface for the windowing device. This acts as the abstraction over the platform
    /// windowing APIs. Most applications will only create one single instance of this class.
    /// Create an instance by calling \ref Device::Create.
    class Device
    {
    public:
        /// Creates a new windowing platform device for running windowed applications.
        ///
        /// \param[in] allocator The allocator to use for all allocations created by this device.
        /// \return The newly created device, or nullptr if there was an error.
        static Device* Create(Allocator& allocator = Allocator::GetDefault());

        /// Destroys a device that was created with \ref Create.
        ///
        /// \param[in] device The device to destroy.
        static void Destroy(Device* device);

    public:
        virtual ~Device() = default;

        /// Gets the allocator used for all allocations in this device.
        ///
        /// \return The allocator.
        Allocator& GetAllocator() { return m_allocator; }

        /// Runs an application and creates a default view. This blocks until \ref Quit is called
        /// and the exit code is returned.
        ///
        /// \param[in] app The application instance to run.
        /// \param[in] desc The descriptor for the default view that will be created.
        /// \return The exit code of the application.
        virtual int Run(Application& app, const ViewDesc& desc) = 0;

        /// Stores the exit code, dispatches a \ref TerminatingEvent, and causes the next event
        /// loop that runs to exit.
        ///
        /// \param[in] exitCode The exit code to return from \ref Run.
        virtual void Quit(int exitCode) = 0;

        /// Returns true if high definition mouse tracking is available.
        ///
        /// \return True if the feature is available, false otherwise.
        virtual const DeviceInfo& GetDeviceInfo() const = 0;

        /// Creates a platform view using the descriptor.
        ///
        /// \param[in] desc The descriptor for the view to create.
        /// \return The new view that was created.
        virtual View* CreateView(const ViewDesc& desc) = 0;

        /// Destroys a view pointer that was created with \ref CreateView.
        ///
        /// \param[in] view The view to destroy.
        virtual void DestroyView(View* view) = 0;

        /// Returns a pointer to the focused view, or nullptr if no view is focused.
        ///
        /// \return The focused view, or nullptr if no view is focused.
        virtual View* FocusedView() const = 0;

        /// Returns a pointer to the hovered view, or nullptr if no view is hovered.
        ///
        /// \return The hovered view, or nullptr if no view is hovered.
        virtual View* HoveredView() const = 0;

        /// Gets the current cursor position. Passing a view pointer will return the coordinates
        /// relative to that view.
        ///
        /// \param[in] view The view to make the coordinates relative to, or nullptr for screen space.
        /// \return The cursor position, or Vec2f_Infinity on error.
        virtual Vec2f GetCursorPos(View* view) const = 0;

        /// Sets the current cursor position. Passing a view pointer will assume the coordinates
        /// are relative to that view.
        ///
        /// \param[in] view The view the coordinates are relative to, or nullptr for screen space.
        /// \param[in] pos The position to set the cursor to.
        virtual void SetCursorPos(View* view, const Vec2f& pos) = 0;

        /// Sets the cursor display mode.
        ///
        /// \param[in] cursor The type of cursor to set it to.
        virtual void SetCursor(PointerCursor cursor) = 0;

        /// Enables or disabled Relative Mode for the cursor.
        /// When enabled Relative Mode hides the cursor and moves it to the center of the screen.
        /// When disabled Relative Mode shows the cursor and restores it to its original position.
        ///
        /// \param[in] relativeMode True to enable relative mode, false to disable.
        virtual void SetCursorRelativeMode(bool relativeMode) = 0;

        /// Returns the number of monitors available to the device.
        ///
        /// \return The number of available monitors.
        virtual uint32_t MonitorCount() const = 0;

        /// Queries the monitor information for the available monitors. You can use
        /// \ref MonitorCount to preallocate the monitors array for input here.
        ///
        /// \param[in] monitors Array of \ref Monitor structures that will be filled in.
        /// \param[in] maxCount Number of \ref Monitor structures in the array that can be written to.
        /// \return The number of available monitors.
        virtual uint32_t GetMonitors(Monitor* monitors, uint32_t maxCount) const = 0;

        /// Retrieves the gamepad interface for the given index.
        ///
        /// \return The gamepad interface.
        virtual Gamepad& GetGamepad(uint32_t index) = 0;

    protected:
        explicit Device(Allocator& allocator) noexcept : m_allocator(allocator) {}

        virtual bool Initialize() = 0;

    protected:
        Allocator& m_allocator;
    };
}
