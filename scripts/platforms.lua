-- Copyright Chad Engler

-- ------------------------------------------------------------------------------------------------
-- Platform extension functions

he._platform_defs = {}
he._platform_names_by_host = {}
he._default_platform_by_host = {}

he.get_platform_names = function (host)
    if host == nil then
        host = os.host()
    end

    return he._platform_names_by_host[host]
end

he.get_platforms = function (host)
    local platform_names = he.get_platform_names(host)
    local platforms = {}

    if platform_names ~= nil then
        for _, name in ipairs(platform_names) do
            table.insert(platforms, he._platform_defs[name])
        end
    end

    return platforms
end

he.add_platform = function (options)
    assert(type(options.name) == "string", "Platform must have a valid name")
    assert(type(options.system) == "string", "Platform must have a valid system name")
    assert(type(options.architecture) == "string", "Platform must have a valid architecture name")
    assert(type(options.hosts) == "table", "Platform must have a valid list of hosts it can be built on")

    assert(he._platform_defs[options.name] == nil, "A platform named '" .. options.name .. "' has already been added.")

    verbosef("Adding platform '" .. options.name .. "' to hosts: " .. table.concat(options.hosts, ", "))

    he._platform_defs[options.name] = options

    for _, host in ipairs(options.hosts) do
        he.enable_platform(host, options.name)
    end
end

he.enable_platform = function (host, platform_name)
    verbosef("Enabling platform '" .. platform_name .. "' for host '" .. host .. "'")
    assert(he._platform_defs[platform_name] ~= nil, "No platform named '" .. platform_name .. "' has been added.")

    local platform = he._platform_defs[platform_name]
    if not table.contains(platform.hosts, host) then
        verbosef("Platform '" .. platform_name .. "' cannot be enabled for host '" .. host .. "' because it is not supported.")
        return
    end

    local platform_names = he._platform_names_by_host[host]

    if platform_names == nil then
        platform_names = {}
    end

    if not table.contains(platform_names, platform_name) then
        table.insert(platform_names, platform_name)
    end

    he._platform_names_by_host[host] = platform_names
end

he.disable_platform = function (host, platform_name)
    verbosef("Disabling platform '" .. platform_name .. "' for host '" .. host .. "'")

    local platform_names = he._platform_names_by_host[host]

    if platform_names ~= nil then
        for i, name in ipairs(platform_names) do
            if name == platform_name then
                table.remove(platforms, i)
                break
            end
        end
    end
end

he.disable_all_platforms = function (host)
    verbosef("Disabling all platforms for host '" .. host .. "'")
    he._platform_names_by_host[host] = {}
end

he.define_platform = function (platform_name)
    verbosef("Defining platform '" .. platform_name .. "'")

    local options = he._platform_defs[platform_name]
    if options ~= nil then
        filter { "platforms:" .. options.name }
            system(options.system)
            architecture(options.architecture)
            if options.on_define ~= nil then
                options.on_define()
            end
        filter {}
    end
end

he.define_platforms = function (host)
    local platform_names = he.get_platform_names(host)
    for _, platform_name in ipairs(platform_names) do
        he.define_platform(platform_name)
    end
end

he.set_default_platform_name = function (host, platform_name)
    verbosef("Setting default platform for host '" .. host .. "' to '" .. platform_name .. "'")
    he._default_platform_by_host[host] = platform_name
end

he.get_default_platform_name = function (host)
    if host == nil then
        host = os.host()
    end

    return he._default_platform_by_host[host]
end

-- ------------------------------------------------------------------------------------------------
-- Add the built-in platforms supported by Harvest

he.add_platform {
    name = "Win64",
    hosts = { "windows" },
    system = "windows",
    architecture = "x86_64",
    on_define = function ()
        vectorextensions "AVX"  -- MSVC has no sse4.1 arch, so we enable AVX
        tags { "simd_AVX" }

        if _OPTIONS.asan == nil then
            editandcontinue "On"
        end
    end,
}

he.add_platform {
    name = "WinARM64",
    hosts = { "windows" },
    system = "windows",
    architecture = "ARM64",
    on_define = function ()
        vectorextensions "NEON"
        tags { "simd_NEON" }
    end,
}

he.add_platform {
    name = "Em32",
    hosts = { "windows", "linux" },
    system = "emscripten",
    architecture = "x86",
    on_define = function ()
        flags { "EmSSE" }
    end,
}

he.add_platform {
    name = "Linux64",
    hosts = { "linux" },
    system = "linux",
    architecture = "x86_64",
    on_define = function ()
        vectorextensions "SSE4.1"
        tags { "simd_SSE4.1" }
    end,
}

he.set_default_platform_name("windows", "Win64")
he.set_default_platform_name("linux", "Linux64")
