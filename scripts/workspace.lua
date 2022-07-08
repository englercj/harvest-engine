-- Copyright Chad Engler

he.workspace = function ()
    workspace(he.sln_name)

    -- Shared project setup
    configurations      { "Debug", "Release", "Shipping" }
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

    -- System setup
    filter { "system:emscripten" }
        defines { "HE_PLATFORM_EMSCRIPTEN", "HE_PLATFORM_API_POSIX" }
        platforms { "emscripten" }
        flags { "EmSSE" }

    filter { "system:linux" }
        defines { "HE_PLATFORM_LINUX", "HE_PLATFORM_API_POSIX" }
        platforms { "x64" }
        buildoptions { "-mcx16", "-fvisibility=hidden", "-fvisibility-inlines-hidden" }

    filter { "system:windows" }
        defines { "HE_PLATFORM_WINDOWS", "HE_PLATFORM_API_WIN32" }
        platforms { "x64", "ARM64" }
        systemversion(_OPTIONS.windows_systemversion)

    -- Platform setup
    filter { "platforms:x64" }
        vectorextensions "AVX"

    filter { "platforms:ARM64" }
        vectorextensions "NEON"

    -- System setup
    filter { "system:windows" }
        defines {
            -- require minimum of windows 10
            "WINVER=0x0A00",
            "_WIN32_WINNT=0x0A00",
            "_ITERATOR_DEBUG_LEVEL=0", -- Improve debug performance
        }

    filter { "system:windows", "platforms:x64" }
        editandcontinue "On"

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
            "-Wundef",      -- A symbol that was not defined was used with a preprocessor directive.
            "-Wswitch",     -- An enumerator has no associated case handler in a switch statement, and there's no default label that can catch it.
        }

    -- Configuration setup
    filter { "configurations:Debug" }
        defines { "_DEBUG", "DEBUG", "HE_CFG_DEBUG" }
        tags { "debug", "internal" }
        inlining "Explicit"
        optimize "Off"
        runtime "Debug"
        symbols "On"

    filter { "configurations:Release" }
        defines { "NDEBUG", "HE_CFG_RELEASE" }
        tags { "release", "internal" }
        flags { "LinkTimeOptimization" }
        inlining "Auto"
        optimize "Speed"
        runtime "Release"
        symbols "On"

    filter { "configurations:Shipping" }
        defines { "NDEBUG", "HE_CFG_SHIPPING" }
        tags { "shipping", "external" }
        flags { "LinkTimeOptimization" }
        inlining "Auto"
        optimize "Speed"
        runtime "Release"
        symbols "Off"

    filter { }
end
