function he_workspace(name)
    workspace(name)

    -- Shared project setup
    configurations      { "Debug", "Release" }
    cppdialect          "C++20"
    cdialect            "C11"
    editandcontinue     "On"
    exceptionhandling   "On" -- TODO: Disable, g3log seems to require it for now.
    flags               { "FatalWarnings", "MultiProcessorCompile" }
    floatingpoint       "Fast"
    location            (build_dir)
    rtti                "Off"
    vectorextensions    "Default"
    warnings            "Extra"

    -- Platform setup
    filter { "system:emscripten" }
        platforms { "emscripten" }

    filter { "system:linux" }
        platforms { "x86_64" }
        buildoptions { "-mcx16", "-fvisibility=hidden", "-fvisibility-inlines-hidden" }

    filter { "system:windows" }
        platforms { "x86_64", "ARM64" }
        systemversion(_OPTIONS.windows_systemversion)

    -- Language setup
    filter { "system:windows", "language:C++" }
        defines {
            -- require minimum of windows 10
            "WINVER=0x0A00",
            "_WIN32_WINNT=0x0A00",
        }

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
