-- Copyright Chad Engler

he._workspace_extension_funcs = {}

he.add_workspace_extension = function (extension_func)
    table.insert(he._workspace_extension_funcs, extension_func)
end

he.workspace = function ()
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
    isaextensions       "RDRND"

    externalanglebrackets   "On"
    externalwarnings        "Off"

    preferredtoolarchitecture "x86_64"

    -- Address Sanitizer setup
    if _OPTIONS.asan ~= nil then
        flags { "NoIncrementalLink" }
        sanitize { "Address" }
    end

    -- Platform setup
    he.define_platforms()

    -- System setup
    filter { "system:emscripten" }
        defines { "HE_PLATFORM_EMSCRIPTEN", "HE_PLATFORM_API_POSIX" }

    filter { "system:linux" }
        defines { "HE_PLATFORM_LINUX", "HE_PLATFORM_API_POSIX" }

    filter { "system:windows" }
        defines {
            "HE_PLATFORM_WINDOWS",
            "HE_PLATFORM_API_WIN32",
            "WINVER=0x0A00",            -- require minimum of windows 10
            "_WIN32_WINNT=0x0A00",      -- require minimum of windows 10
            "_ITERATOR_DEBUG_LEVEL=0",  -- Improve debug performance by disabling iterator checks
        }
        systemversion(_OPTIONS.windows_systemversion)

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
            "-mcx16",                       -- Enable use of CMPXCHG16B for 16-byte aligned 128-bit objects
            "-mxsave",                      -- Enable use of xsave/xrstor instructions
            "-fPIC",                        -- Generate position-independent code
            "-fvisibility=hidden",          -- Mark all symbols as hidden by default
            "-Wundef",                      -- A symbol that was not defined was used with a preprocessor directive.
            "-Wswitch",                     -- An enumerator has no associated case handler in a switch statement, and there's no default label that can catch it.
        }

    -- Should really be using "language:c++" here instead of the file filter,
    -- but doing so still includes this in CFLAGS which causes an error.
    filter { "toolset:gcc or clang", "files:**.cpp or **.cc" }
        buildoptions {
            "-fvisibility-inlines-hidden",  -- Hide inlines from the symbol table
        }

    filter { "toolset:emcc" }
        em_options {
            "ALLOW_MEMORY_GROWTH=1",        -- Enable dynamic memory growth of the WASM memory buffer
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
