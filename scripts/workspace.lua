-- Copyright Chad Engler

he._workspace_extension_funcs = {}

he.add_workspace_extension = function (extension_func)
    table.insert(he._workspace_extension_funcs, extension_func)
end

he.generate_workspace = function (options)
    verbosef("Creating workspace: " .. he.sln_name)
    workspace(he.sln_name)

    -- Shared project setup
    configurations      { "Debug", "Development", "Release" }
    platforms           (he.get_platform_names())
    defaultplatform     (he.get_default_platform_name())
    cppdialect          "C++20"
    cdialect            "C11"
    editandcontinue     "Off"
    exceptionhandling   "Off"
    flags               { "FatalWarnings", "MultiProcessorCompile" }
    floatingpoint       "Fast"
    location            (he.build_dir)
    warnings            "Extra"

    externalanglebrackets   "On"
    externalwarnings        "Off"

    preferredtoolarchitecture "x86_64"

    startproject(options.start_project)

    -- Address Sanitizer setup
    if _OPTIONS.asan ~= nil then
        flags { "NoIncrementalLink" }
        sanitize { "Address" }
        tags { "asan" }
    end

    -- Platform setup
    he.define_platforms()

    filter { "architecture:x86 or x86_64" }
        isaextensions { "RDRND" }

    -- System setup
    filter { "system:windows" }
        defines {
            "WINVER=0x0A00",            -- require minimum of windows 10
            "_WIN32_WINNT=0x0A00",      -- require minimum of windows 10
            "_ITERATOR_DEBUG_LEVEL=0",  -- Improve debug performance by disabling iterator checks
        }
        systemversion(_OPTIONS.windows_systemversion)

    filter { "system:wasm" }
        wasmoptpath(he.get_wasm_opt_path())

    -- Compiler setup
    filter { "toolset:msc-*" }
        buildoptions {
            "/permissive-", -- Enable standards-conforming compiler behavior.
            "/utf-8",       -- Specifies both the source character set and the execution character set as UTF-8.
            "/w44668",      -- A symbol that was not defined was used with a preprocessor directive.
            "/w44062",      -- An enumerator has no associated case handler in a switch statement, and there's no default label that can catch it.
            "/wd6255",      -- _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead.
        }

    filter { "toolset:gcc or clang" }
        buildoptions {
            "-mcx16",               -- Enable use of CMPXCHG16B for 16-byte aligned 128-bit objects
            "-mxsave",              -- Enable use of xsave/xrstor instructions
        }

    filter { "toolset:gcc or clang or wasmcc" }
        buildoptions {
            "-fPIC",                -- Generate position-independent code
            "-fvisibility=hidden",  -- Mark all symbols as hidden by default
            "-Wundef",              -- A symbol that was not defined was used with a preprocessor directive.
            "-Wswitch",             -- An enumerator has no associated case handler in a switch statement, and there's no default label that can catch it.
        }

    filter { "toolset:wasmcc" }
        buildoptions {
            "-nostdlib",                                    -- Do not use the standard system startup files or libraries when linking.
        }
        -- buildoptions(he.get_emscripten_wasm_include_flags())
        buildoptions(he.get_libcxx_wasm_include_flags())
        linkoptions {
            "-Wl,--export-dynamic",     -- Export any non-hidden symbols.
            "-Wl,--fatal-warnings",     -- Emit an error when any warning is encountered.
            "-Wl,--import-memory",      -- Import memory from the environment.
            "-Wl,--no-entry",           -- Don't search for the entry point symbol (by default _start).
            "-Wl,-export=main",         -- Export the main function so we can invoke it as the entry point.
        }

    -- Should really be using "language:c++" here instead of the file filter,
    -- but doing so still includes this in CFLAGS which causes an error.
    filter { "toolset:gcc or clang or wasmcc", "files:**.cpp or **.cc" }
        buildoptions {
            "-fvisibility-inlines-hidden",  -- Hide inlines from the symbol table
        }

    -- Configuration setup
    filter { "configurations:Debug" }
        defines { "_DEBUG", "DEBUG", "HE_CFG_DEBUG" }
        tags { "debug", "internal" }
        inlining "Disabled"
        optimize "Off"
        runtime "Debug"
        symbols "Full"

    filter { "configurations:Development" }
        defines { "NDEBUG", "HE_CFG_DEVELOPMENT" }
        tags { "development", "internal" }
        inlining "Explicit"
        optimize "On"
        runtime "Release"
        symbols "On"

    filter { "configurations:Release" }
        defines { "NDEBUG", "HE_CFG_RELEASE" }
        tags { "release", "external" }
        flags { "LinkTimeOptimization" }
        inlining "Auto"
        optimize "Speed"
        runtime "Release"
        symbols "On"

    filter { }

    for _, func in ipairs(he._workspace_extension_funcs) do
        if func ~= nil then
            func()
        end
    end
end
