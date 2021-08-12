-- Copyright Chad Engler

function he_workspace()
    workspace(sln_name)

    -- Shared project setup
    configurations      { "Debug", "Release", "Shipping" }
    cppdialect          "C++20"
    cdialect            "C11"
    editandcontinue     "Off"
    exceptionhandling   "Off"
    flags               { "FatalWarnings", "MultiProcessorCompile" }
    floatingpoint       "Fast"
    location            (build_dir)
    rtti                "Off"
    warnings            "Extra"

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

    -- Language setup
    filter { "system:windows" }
        defines {
            -- require minimum of windows 10
            "WINVER=0x0A00",
            "_WIN32_WINNT=0x0A00",
            "_ITERATOR_DEBUG_LEVEL=0", -- Improve debug performance
        }

    -- Compiler setup
    filter { "toolset:msc-*" }
        buildoptions {
            "/we4668",      -- A symbol that was not defined was used with a preprocessor directive.
            "/permissive-", -- Enable standards-conforming compiler behavior.
            "/utf-8",       -- Specifies both the source character set and the execution character set as UTF-8.
        }

    filter { "toolset:gcc or clang" }
        buildoptions {
            "-Wundef", -- A symbol that was not defined was used with a preprocessor directive.
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
        tags { "release" }
        flags { "LinkTimeOptimization" }
        inlining "Auto"
        optimize "Speed"
        runtime "Release"
        symbols "Off"

    filter { }
end
