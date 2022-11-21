// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    /// Gets the contents of the environment variable of `name` from the environment
    /// block of the calling process.
    ///
    /// \param[in] name The name of the environment variable to read.
    /// \return The contents of the environment variable, or an empty string if an error occurred.
    String GetEnv(const char* name, Allocator& allocator = Allocator::GetDefault());

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
}
