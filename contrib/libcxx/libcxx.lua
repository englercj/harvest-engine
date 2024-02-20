-- Copyright Chad Engler

return function (plugin, install_dirs)
    function he.get_libcxx_abi_version()
        return 2
    end

    local install_dir = install_dirs["*"]
    local config_path = path.join(install_dir, "include", "__config_site")
    local config_lines = {}

    table.insert(config_lines, [[
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CONFIG_SITE
#define _LIBCPP___CONFIG_SITE
]])

    table.insert(config_lines, "#define _LIBCPP_ABI_VERSION " .. he.get_libcxx_abi_version())
    table.insert(config_lines, "#define _LIBCPP_ABI_NAMESPACE __" .. he.get_libcxx_abi_version())
    -- table.insert(config_lines, "#define _LIBCPP_ABI_FORCE_ITANIUM")
    -- table.insert(config_lines, "#define _LIBCPP_ABI_FORCE_MICROSOFT")
    table.insert(config_lines, "#define _LIBCPP_HAS_NO_THREADS")
    -- table.insert(config_lines, "#define _LIBCPP_HAS_NO_MONOTONIC_CLOCK")
    -- table.insert(config_lines, "#define _LIBCPP_HAS_MUSL_LIBC")
    -- table.insert(config_lines, "#define _LIBCPP_HAS_THREAD_API_PTHREAD")
    -- table.insert(config_lines, "#define _LIBCPP_HAS_THREAD_API_EXTERNAL")
    -- table.insert(config_lines, "#define _LIBCPP_HAS_THREAD_API_WIN32")
    -- table.insert(config_lines, "#define _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS")
    table.insert(config_lines, "#define _LIBCPP_HAS_NO_VENDOR_AVAILABILITY_ANNOTATIONS")
    table.insert(config_lines, "#define _LIBCPP_NO_VCRUNTIME")
    -- table.insert(config_lines, "#define _LIBCPP_TYPEINFO_COMPARISON_IMPLEMENTATION default")
    table.insert(config_lines, "#define _LIBCPP_HAS_NO_FILESYSTEM")
    table.insert(config_lines, "#define _LIBCPP_HAS_NO_RANDOM_DEVICE")
    table.insert(config_lines, "#define _LIBCPP_HAS_NO_LOCALIZATION")
    -- table.insert(config_lines, "#define _LIBCPP_HAS_NO_WIDE_CHARACTERS")
    table.insert(config_lines, "#define _LIBCPP_ENABLE_ASSERTIONS_DEFAULT 0")
    table.insert(config_lines, "#define _LIBCPP_AVAILABILITY_HAS_NO_VERBOSE_ABORT")

    table.insert(config_lines, "\n// PSTL backends")
    table.insert(config_lines, "#define _LIBCPP_PSTL_CPU_BACKEND_SERIAL 1")
    -- table.insert(config_lines, "#define _LIBCPP_PSTL_CPU_BACKEND_THREAD 1")
    -- table.insert(config_lines, "#define _LIBCPP_PSTL_CPU_BACKEND_LIBDISPATCH 1")

    table.insert(config_lines, "\n// Hardening.")
    table.insert(config_lines, "#define _LIBCPP_ENABLE_HARDENED_MODE_DEFAULT 0")
    table.insert(config_lines, "#define _LIBCPP_ENABLE_DEBUG_MODE_DEFAULT 0")

    table.insert(config_lines, [[

#endif // _LIBCPP___CONFIG_SITE]])

    local file = assert(io.open(config_path, "w"))
    for _, line in ipairs(config_lines) do
        file:write(line)
        file:write("\n")
    end
    file:close()
end
