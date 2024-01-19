-- Copyright Chad Engler

-- ------------------------------------------------------------------------------------------------
-- Platform extension functions

he._platform_defs = {}
he._platform_names_by_host = {}
he._default_platform_by_host = {}

function he.get_platform_names(host)
    if host == nil then
        host = os.host()
    end

    return he._platform_names_by_host[host]
end

function he.get_platforms(host)
    local platform_names = he.get_platform_names(host)
    local platforms = {}

    if platform_names ~= nil then
        for _, name in ipairs(platform_names) do
            table.insert(platforms, he._platform_defs[name])
        end
    end

    return platforms
end

function he.add_platform(options)
    assert(type(options.name) == "string", "Platform must have a valid name")
    assert(type(options.system) == "string", "Platform must have a valid system name")
    assert(type(options.architecture) == "string", "Platform must have a valid architecture name")
    assert(type(options.hosts) == "table", "Platform must have a valid list of hosts it can be built on")

    assert(he._platform_defs[options.name] == nil, "A platform named '" .. options.name .. "' has already been added.")

    verbosef("Adding platform '" .. options.name .. "' to hosts: " .. table.concat(options.hosts, ", "))

    he._platform_defs[options.name] = options
end

function he.enable_platform(host, platform_name)
    assert(he._platform_defs[platform_name] ~= nil, "No platform named '" .. platform_name .. "' has been added.")
    local platform = he._platform_defs[platform_name]

    if not table.contains(platform.hosts, host) then
        verbosef("Platform '" .. platform_name .. "' cannot be enabled for host '" .. host .. "' because it is not supported.")
        return
    end

    if platform.can_enable ~= nil and not platform.can_enable(host) then
        verbosef("Platform '" .. platform_name .. "' returned false for can_enable().")
        return
    end

    verbosef("Enabling platform '" .. platform_name .. "' for host '" .. host .. "'")

    local platform_names = he._platform_names_by_host[host]

    if platform_names == nil then
        platform_names = {}
    end

    if not table.contains(platform_names, platform_name) then
        table.insert(platform_names, platform_name)
    end

    he._platform_names_by_host[host] = platform_names
end

function he.enable_all_platforms(host)
    verbosef("Enabling all platforms for host '" .. host .. "'")

    for name, platform in pairs(he._platform_defs) do
        if table.contains(platform.hosts, host) then
            he.enable_platform(host, name)
        end
    end
end

function he.disable_platform(host, platform_name)
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

function he.disable_all_platforms(host)
    verbosef("Disabling all platforms for host '" .. host .. "'")
    he._platform_names_by_host[host] = {}
end

function he.define_platform(platform_name)
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

function he.define_platforms(host)
    local platform_names = he.get_platform_names(host)
    for _, platform_name in ipairs(platform_names) do
        he.define_platform(platform_name)
    end
end

function he.set_default_platform_name(host, platform_name)
    verbosef("Setting default platform for host '" .. host .. "' to '" .. platform_name .. "'")
    he._default_platform_by_host[host] = platform_name
end

function he.get_default_platform_name(host)
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
        vectorextensions "AVX"
        defines { "HE_PLATFORM_WINDOWS", "HE_PLATFORM_API_WIN32" }

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
        defines { "HE_PLATFORM_WINDOWS", "HE_PLATFORM_API_WIN32" }
    end,
}

local function _can_enable_wasm()
    local clang_path = he.whereis("clang")
    local clangpp_path = he.whereis("clang++")
    local wasm_ld_path = he.whereis("wasm-ld")
    local supported = clang_path ~= nil and clangpp_path ~= nil and wasm_ld_path ~= nil

    if not supported then
        premake.warn("WASM platforms cannot be enabled because clang, clang++, and/or wasm-ld could not be found.")
        premake.warn("To enable this platform install LLVM: https://releases.llvm.org/download.html")
    end

    return supported
end

he.add_platform {
    name = "Wasm32",
    hosts = { "windows", "linux" },
    system = "wasm",
    architecture = "wasm32",
    on_define = function ()
        vectorextensions "SIMD128"
        wasmfeatures { "Atomics", "BulkMemory", "NontrappingFPToInt" }
        defines { "HE_PLATFORM_WASM" }
    end,
    can_enable = _can_enable_wasm,
}

he.add_platform {
    name = "Wasm64",
    hosts = { "windows", "linux" },
    system = "wasm",
    architecture = "wasm64",
    on_define = function ()
        vectorextensions "SIMD128"
        wasmfeatures { "Atomics", "BulkMemory", "NontrappingFPToInt" }
        defines { "HE_PLATFORM_WASM" }
    end,
    can_enable = _can_enable_wasm,
}

he.add_platform {
    name = "Linux64",
    hosts = { "linux" },
    system = "linux",
    architecture = "x86_64",
    on_define = function ()
        vectorextensions "SSE4.2"
        defines { "HE_PLATFORM_LINUX", "HE_PLATFORM_API_POSIX" }
    end,
}

he.set_default_platform_name("windows", "Win64")
he.set_default_platform_name("linux", "Linux64")
