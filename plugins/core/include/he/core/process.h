// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    /// Gets the contents of the environment variable of `name` from the environment block of the
    /// calling process. If the variable does not exist in the environment an error is returned.
    ///
    /// \param[in] name The name of the environment variable to read.
    /// \param[out] outValue The value of the environment variable that was read.
    /// \return The result of the operation.
    Result GetEnv(const char* name, String& outValue);

    /// Sets an enviroment variable.
    ///
    /// \note If the environment variable already exists, it will be overwritten with the new value.
    ///
    /// \param[in] name The name of the environment variable to set.
    /// \param[in] value The value to set the environment variable to. If nullptr, the environment
    ///     variable will be unset.
    /// \return The result of the operation from the OS.
    Result SetEnv(const char* name, const char* value);

    /// Gets the process ID of the calling process.
    ///
    /// \return The process ID.
    uint32_t GetCurrentProcessId();

    /// Checks if a given process ID is currently running.
    ///
    /// \param[in] pid The process ID to check.
    /// \return True if a process is running under `pid`, false otherwise.
    bool IsProcessRunning(uint32_t pid);

    /// Gets the filename of the executable that started the calling process.
    ///
    /// \param[out] out The absolute path to the process executable file.
    /// \return The result of the operation from the OS.
    Result GetCurrentProcessFilename(String& out);

    /// Terminates the calling process immediately.
    [[noreturn]] void TerminateProcess();
}
