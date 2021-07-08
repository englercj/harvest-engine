-- Copyright Chad Engler

function he_workspace(name)
    workspace(name)

    -- Shared project setup
    configurations      { "Debug", "Release" }
    cppdialect          "C++20"
    cdialect            "C11"
    editandcontinue     "On"
    exceptionhandling   "Off"
    flags               { "FatalWarnings", "MultiProcessorCompile" }
    floatingpoint       "Fast"
    location            (build_dir)
    rtti                "Off"
    vectorextensions    "SSE4.1"
    warnings            "Extra"

    -- System setup
    filter { "system:emscripten" }
        platforms { "emscripten" }
        flags { "EmSSE" }

    filter { "system:linux" }
        platforms { "x64" }
        buildoptions { "-mcx16", "-fvisibility=hidden", "-fvisibility-inlines-hidden" }

    filter { "system:windows" }
        platforms { "x64", "ARM64" }
        systemversion(_OPTIONS.windows_systemversion)

    -- Language setup
    filter { "system:windows", "language:C++" }
        defines {
            -- require minimum of windows 10
            "WINVER=0x0A00",
            "_WIN32_WINNT=0x0A00",
        }

    -- Platform setup
    filter { "system:windows", "architecture:ARM64" }
        editandcontinue "Off"
        vectorextensions "NEON"

    -- Compiler setup
    filter { "toolset:msc-*", "language:C++" }
        buildoptions { "/permissive-" }

    -- Configuration setup
    filter { "configurations:Debug" }
        defines { "_DEBUG", "DEBUG" }
        tags { "debug" }
        inlining "Explicit"
        optimize "Off"
        runtime "Debug"
        symbols "On"

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        tags { "release" }
        flags { "LinkTimeOptimization" }
        inlining "Auto"
        optimize "Speed"
        runtime "Release"
        symbols "Off"

    filter { }
end
