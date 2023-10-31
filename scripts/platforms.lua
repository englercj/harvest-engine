-- Copyright Chad Engler

-- ------------------------------------------------------------------------------------------------
-- Platform extension functions

he._platforms_by_host = {}
he._platform_definitions = {}
he._default_platform_by_host = {}

he.get_platforms = function (host)
    if host == nil then
        host = os.host()
    end

    return he._platforms_by_host[host]
end

he.add_platform = function (host, platform_name, definition_func)
    assert(he._platform_definitions[platform_name] == nil, "There is already a platform named '" .. platform_name .. "'")
    verbosef("Adding platform '" .. platform_name .. "' to host '" .. host .. "'")

    local platforms = he._platforms_by_host[host]

    if platforms == nil then
        platforms = {}
    end

    table.insert(platforms, platform_name)
    he._platforms_by_host[host] = platforms
    he._platform_definitions[platform_name] = definition_func
end

he.remove_platform = function (host, platform_name)
    verbosef("Removing platform '" .. platform_name .. "' from host '" .. host .. "'")

    local platforms = he._platforms_by_host[host]

    if platforms ~= nil then
        for i, existing_name in ipairs(platforms) do
            if existing_name == platform_name then
                table.remove(platforms, i)
                break
            end
        end
    end

    he._platform_definitions[platform_name] = nil
end

he.define_platform = function (platform_name)
    local definition_func = he._platform_definitions[platform_name]
    if definition_func ~= nil then
        filter { "platforms:" .. platform_name }
            definition_func()
        filter {}
    end
end

he.define_platforms = function (host)
    if host == nil then
        host = os.host()
    end

    local platforms = he.get_platforms(host)
    for _, platform_name in ipairs(platforms) do
        he.define_platform(platform_name)
    end
end

he.set_default_platform = function (host, platform_name)
    verbosef("Setting default platform for host '" .. host .. "' to '" .. platform_name .. "'")
    he._default_platform_by_host[host] = platform_name
end

he.get_default_platform = function (host)
    if host == nil then
        host = os.host()
    end

    return he._default_platform_by_host[host]
end

-- ------------------------------------------------------------------------------------------------
-- Add the built-in platforms supported by Harvest

he.add_platform("windows", "Win64", function ()
    system "windows"
    architecture "x86_64"
    vectorextensions "AVX"  -- MSVC has no sse4.1 arch, so we enable AVX
    tags { "simd_AVX" }

    if _OPTIONS.asan == nil then
        editandcontinue "On"
    end
end)

he.add_platform("windows", "WinARM64", function ()
    system "windows"
    architecture "ARM64"
    vectorextensions "NEON"
    tags { "simd_NEON" }
end)

he.add_platform("windows", "Em32", function ()
    system "emscripten"
    architecture "x86"
    flags { "EmSSE" }
end)

he.set_default_platform("windows", "Win64")

he.add_platform("linux", "Linux64", function ()
    system "linux"
    architecture "x86_64"
    vectorextensions "SSE4.1"
    tags { "simd_SSE4.1" }
end)

he.set_default_platform("windows", "Linux64")
