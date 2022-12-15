// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    // --------------------------------------------------------------------------------------------

    /// Structure storing information about the system.
    /// \see GetSystemInfo()
    struct SystemInfo
    {
        /// The granularity for the starting address at which virtual memory can be allocated.
        uint32_t allocationGranularity{ 0 };

        /// The page size and the granularity of page protection and commitment.
        /// This is the page size used by the VirtualMemory class.
        uint32_t pageSize{ 0 };

        /// The name of the platform as reported by the platform itself.
        String platform{};

        /// Version of the platform.
        struct
        {
            uint32_t major{ 0 };
            uint32_t minor{ 0 };
            uint32_t patch{ 0 };
            uint32_t build{ 0 };
        } version{};
    };

    /// Structure storing power information about the system.
    /// \see GetPowerStatus
    struct PowerStatus
    {
        /// Helper container for a PowerStatus value that tracks a value and if it is valid.
        template <typename T>
        struct Value
        {
            T value{};
            bool valid{ false };

            /// \internal
            void Set(T value_) { value = value_; valid = true; }

            /// \internal
            void Clear() { valid = false; }
        };

        /// When true the system is on AC power (not battery).
        Value<bool> onACPower{};

        /// When true the system has a battery (though it may be on AC power).
        Value<bool> hasBattery{};

        /// Percent battery life remaining. Ranges of [0, 100].
        /// Invalid if there is no battery.
        Value<uint8_t> batteryLife{};

        /// Duration of battery life remaining.
        /// Invalid if there is no battery, or on AC power.
        Value<Duration> batteryLifeTime{};
    };

    /// Gets basic information about the system.
    const SystemInfo& GetSystemInfo();

    /// Gets the name of the system and writes it into `outName`.
    /// On windows this is the NetBIOS name.
    /// On posix this is the host name.
    ///
    /// \param[out] outName The string to fill with the name of the system.
    /// \return The result of the operation.
    Result GetSystemName(String& outName);

    /// Gets the name of the system user who is associated with the current process, and writes
    /// it to `outName`.
    ///
    /// \param[out] outName The string to fill with the name of the user.
    /// \return The result of the operation.
    Result GetSystemUserName(String& outName);

    /// Gets the power status of the system.
    ///
    /// \return Current power status of the system.
    PowerStatus GetPowerStatus();

    /// Load a dynamic library and return a handle to access symbols in it.
    ///
    /// \param[in] path The path to the object to load.
    /// \return A handle to the opened dynamic library.
    void* DLOpen(const char* path);

    /// Close the dynamic library handle. The handle should no longer be used after calling
    /// this function.
    ///
    /// \param[in] handle The handle to close.
    void DLClose(void* handle);

    /// Lookup a symbol in an open dynamic library and return a pointer to the symbol in memory.
    ///
    /// \param[in] handle A handle to the dynamic library to search within. \see DLOpen
    /// \param[in] symbol The name of the symbol to look for.
    /// \return A pointer to the symbol in memory, or nullptr if not found.
    void* DLSymbol(void* handle, const char* symbol);

    /// Lookup a symbol in an open dynamic library and return a pointer to the symbol in memory.
    ///
    /// \param[in] handle A handle to the dynamic library to search within. \see DLOpen
    /// \param[in] symbol The name of the symbol to look for.
    /// \return A pointer to the symbol in memory, or nullptr if not found.
    template <typename T>
    T DLSymbol(void* handle, const char* symbol)
    {
        return reinterpret_cast<T>(DLSymbol(handle, symbol));
    }

    // --------------------------------------------------------------------------------------------

    /// Helper class to manage the lifetime of a dynamic library. Stores the handle and
    /// automatically closes it when the class is destructed.
    class DynamicLib
    {
    public:
        DynamicLib() = default;

        DynamicLib(const DynamicLib& x) = delete;

        DynamicLib(DynamicLib&& x) noexcept
            : m_handle(Exchange(x.m_handle, nullptr))
        {}

        ~DynamicLib() noexcept { Close(); }

        DynamicLib& operator=(const DynamicLib&) = delete;

        DynamicLib& operator=(DynamicLib&& x)
        {
            Close();
            m_handle = Exchange(x.m_handle, nullptr);
            return *this;
        }

        explicit operator bool() const { return IsOpen(); }

        /// Checks if this library is currently open.
        ///
        /// \return True if the library is open, false otherwise.
        bool IsOpen() const { return m_handle != nullptr; }

        /// Load a dynamic library and stores a handle to access symbols in it.
        ///
        /// \param[in] path The path to the object to load.
        void Open(const char* path)
        {
            Close();
            m_handle = DLOpen(path);
        }

        /// Close the dynamic library handle.
        void Close()
        {
            if (m_handle)
            {
                DLClose(m_handle);
                m_handle = nullptr;
            }
        }

        /// Lookup a symbol in an open dynamic library and return a pointer to the symbol in memory.
        ///
        /// \param[in] symbol The name of the symbol to look for.
        /// \return A pointer to the symbol in memory, or nullptr if not found.
        void* Symbol(const char* symbol)
        {
            return DLSymbol(m_handle, symbol);
        }

        /// Lookup a symbol in an open dynamic library and return a pointer to the symbol in memory.
        ///
        /// \param[in] symbol The name of the symbol to look for.
        /// \return A pointer to the symbol in memory, or nullptr if not found.
        template <typename T>
        T Symbol(const char* symbol)
        {
            return DLSymbol<T>(m_handle, symbol);
        }

    private:
        void* m_handle{ nullptr };
    };
}
