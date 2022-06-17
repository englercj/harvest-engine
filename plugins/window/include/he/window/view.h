// Copyright Chad Engler

#pragma once

#include "he/core/enum_ops.h"
#include "he/math/types.h"

namespace he::window
{
    /// A view hit area is a descriptor of special parts of the application that can relay
    /// information about user interactions to the OS. This is used when the view is
    /// borderless to allow the application to display custom UI for controling the view,
    /// but still allow the OS to handle the behavior of interacting with that custom UI.
    enum class ViewHitArea : uint8_t
    {
        ButtonClose,                ///< The app close button.
        ButtonMinimize,             ///< The app minimize button.
        ButtonMaximizeAndRestore,   ///< The maximize/restore app button.
        Draggable,                  ///< A draggable area in the client view.
        Normal,                     ///< Within the view, but not a special area.
        NotInView,                  ///< The point is outside the view.
        ResizeTopLeft,              ///< The top-left resize handle.
        ResizeTop,                  ///< The top resize handle.
        ResizeTopRight,             ///< The top-right resize handle.
        ResizeRight,                ///< The right resize handle.
        ResizeBottomRight,          ///< The bottom-right resize handle.
        ResizeBottom,               ///< The bottom resize handle.
        ResizeBottomLeft,           ///< The bottom-left resize handle.
        ResizeLeft,                 ///< The left resize handle.
        SystemMenu,                 ///< The app system menu button.
    };

    /// Flags controlling the behavior of a view.
    enum class ViewFlag : uint32_t
    {
        None            = 0,
        AcceptFiles     = 1 << 0,   ///< Can accept drag/dropped files
        AcceptInput     = 1 << 1,   ///< Can accept mouse input & focus
        Borderless      = 1 << 2,   ///< Disabled OS borders and title bar
        FocusOnClick    = 1 << 3,   ///< Enables focus when clicked
        AllowResize     = 1 << 4,   ///< Can be resized, minimized, and maximized
        StartMaximized  = 1 << 5,   ///< Starts the windows maximized
        TaskBarIcon     = 1 << 6,   ///< Display a task bar icon
        TopMost         = 1 << 7,   ///< Set as the top-most window

        /// The default flags used for creating a view.
        Default = AcceptInput | FocusOnClick | AllowResize | TaskBarIcon,
    };
    HE_ENUM_FLAGS(ViewFlag);

    /// Descriptor for creating a new view.
    struct ViewDesc
    {
        /// String to display as the title of the view, if it is not borderless.
        const char* title{ nullptr };

        /// Opaque user data stored on the view. Can be retrieved later with \ref View::GetUserData
        void* userData{ nullptr };

        /// Starting size of the view when created.
        Vec2i size{ 0, 0 };

        /// Starting position of the view when created.
        Vec2i pos{ 0, 0 };

        /// Flags controlling the behavior of the view.
        ViewFlag flags{ ViewFlag::Default };

        /// The parent view for this one when spawning a child view.
        class View* parent{ nullptr };
    };

    /// Interface for controlling a spawned platform view.
    class View
    {
    public:
        virtual ~View() {}

        /// Gets the native OS view handle for this view. For example, this is an HWND on Windows.
        ///
        /// \return The native OS view handle.
        virtual void* GetNativeHandle() const = 0;

        /// Gets the opaque user data pointer that was provided in the \ref ViewDesc when this
        /// view was created.
        ///
        /// \return The user data pointer.
        virtual void* GetUserData() const = 0;

        /// Gets the current position of the view.
        ///
        /// \return The position of the view in screen space.
        virtual Vec2i GetPosition() const = 0;

        /// Gets the current size of the view.
        ///
        /// \return The size of the view.
        virtual Vec2i GetSize() const = 0;

        /// Gets the current DPI scale of the view, where 1.0 is 96 DPI.
        ///
        /// \return The size of the view.
        virtual float GetDpiScale() const = 0;

        /// Checks if this view is the focused view of the application.
        ///
        /// \return True if this is the focused view, false otherwise.
        virtual bool IsFocused() const = 0;

        /// Checks if this view or any child is the focused view of the application.
        ///
        /// \return True if this or any child is the focused view, false otherwise.
        virtual bool IsChildFocused() const = 0;

        /// Checks if this view is minimized.
        ///
        /// \return True if this view is minimized.
        virtual bool IsMinimized() const = 0;

        /// Checks if this view is maximized.
        ///
        /// \return True if this view is maximized.
        virtual bool IsMaximized() const = 0;

        /// Sets the position of the view.
        ///
        /// \param[in] pos The position in screen space to move the view to.
        virtual void SetPosition(const Vec2i& pos) = 0;

        /// Sets the size of the view.
        ///
        /// \param[in] pos The size to change the view to.
        virtual void SetSize(const Vec2i& size) = 0;

        /// Shows or hides the view, optionally focusing the view when showing it.
        ///
        /// \param[in] visible Pass true to show the view, or false to hide it.
        /// \param[in] focus When `visible` is true, this controls if the view is focused upon appearing.
        virtual void SetVisible(bool visible, bool focus = false) = 0;

        /// Sets the title of the view for the platform. This has no visible effect for
        /// borderless views
        ///
        /// \param[in] text The string to set the title to.
        virtual void SetTitle(const char* text) = 0;

        /// Sets the opacity of the view. A value of 0.0 is fully transparent, and a value
        /// of 1.0 is fully opaque.
        ///
        /// \param[in] alpha The normalized alpha value to set the window to.
        virtual void SetAlpha(float alpha) = 0;

        /// Enable or disable the view accepting input.
        ///
        /// \param[in] value True if the view should accept input, false otherwise.
        virtual void SetAcceptInput(bool value) = 0;

        /// Causes this view to become the active, focused, view of the application.
        virtual void Focus() = 0;

        /// Minimizes the view to the task bar.
        virtual void Minimize() = 0;

        /// Maximizes the view to fill the screen.
        virtual void Maximize() = 0;

        /// Restores the view to a normal windowed state. This can restore a window from the
        /// Minimized or Maximized states.
        virtual void Restore() = 0;

        /// Requests that the view closes itself. Generally this posts a message into the queue
        /// to be processed on the next event loop, and doesn't close the view immediately.
        virtual void RequestClose() = 0;

        /// Converts the coordinates from view space to screen space.
        ///
        /// \param[in] pos The position relative to this view to convert.
        /// \return The converted screen space coordinates.
        virtual Vec2f ViewToScreen(const Vec2f& pos) const = 0;

        /// Converts the coordinates from screen space to view space.
        ///
        /// \param[in] pos The screen space position to convert.
        /// \return The converted view space coordinates.
        virtual Vec2f ScreenToView(const Vec2f& pos) const = 0;

        /// Toggles the maximized state of the view. If the view is maximized this has the
        /// effect of calling \ref Restore. Otherwise it has the effect of calling \ref Maximize.
        void ToggleMaximize() { if (IsMaximized()) Restore(); else Maximize(); }
    };
}
