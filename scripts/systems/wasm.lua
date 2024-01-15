-- Copyright Chad Engler

local p = premake
local api = p.api
local config = p.config

--
-- Register the WASM extensions
--

api.addAllowed("system", { "wasm" })
api.addAllowed("architecture", { "wasm32" })
api.addAllowed("toolset", { "wasmcc" })
api.addAllowed("vectorextensions", { "SIMD128" })

-- Path to the wasm-opt executable used for optimizing the final binary.
-- Can also specify this in the environment variable "WASM_OPT_PATH".
api.register {
    name = "wasmoptpath",
    scope = "config",
    kind = "path",
    tokens = true,
}

-- Path to search for libc/libcxx prepared for Wasm.
api.register {
    name = "wasmsyspath",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "wasmfeatures",
    scope = "config",
    kind = "list:string",
    allowed = {
        "Atomics",
        "BulkMemory",
    },
}

local os_option = p.option.get("os")
if os_option ~= nil then
    table.insert(os_option.allowed, { "wasm" })
end

os.systemTags["wasm"] = { "wasm", "web" }

--
-- Setup some sane defaults for WASM system
--

filter { "system:wasm" }
    architecture("wasm32")
    toolset("wasmcc")

filter { "system:wasm", "kind:StaticLib" }
    targetextension ".bc"

filter { "system:wasm", "kind:SharedLib" }
    targetextension ".bc"

filter { "system:wasm", "kind:ConsoleApp" }
    targetextension ".js"

filter { "system:wasm", "kind:WindowedApp" }
    targetextension ".html"

filter { "system:wasm", "kind:ConsoleApp or WindowedApp" }
    -- Run the optimizer on the final binary
    postbuildmessage "Optimizing wasm binary file %{cfg.linktarget.name}"
    postbuildcommands {
        "%{premake.tools.wasmcc._get_wasm_opt_path(cfg)} %{cfg.linktarget.abspath} %{premake.tools.wasmcc._wasm_opt_args(prj, cfg)} -o %{cfg.linktarget.abspath}",
    }

filter {}

--
-- Setup the wasmcc toolset
-- This is just clang, but using wasm-ld for linking and a few extra wasm flags
--

local clang = premake.tools.clang
local wasmcc = {}

wasmcc.getrunpathdirs = clang.getrunpathdirs

wasmcc.shared = table.merge(clang.shared, {
    architecture = {
        wasm32 = "-target wasm32",
    },
    wasmfeatures = {
        Atomics = "-matomics",
        BulkMemory = "-mbulk-memory",
    },
    vectorextensions = {
        SIMD128 = "-msimd128",
    },
})

wasmcc.cflags = table.merge(clang.cflags, {
})

wasmcc.cxxflags = table.merge(clang.cxxflags, {
})

function wasmcc.getcflags(cfg)
    local shared = config.mapFlags(cfg, wasmcc.shared)
    local cflags = config.mapFlags(cfg, wasmcc.cflags)
    local warnings = wasmcc.getwarnings(cfg)
    local sysversion = wasmcc.getsystemversionflags(cfg)
    return table.join(shared, cxxflags, warnings, sysversion)
end

function wasmcc.getcppflags(cfg)
    local flags = clang.getcppflags(cfg)
    table.insert(flags, "-nostdinc")
    return flags
end

function wasmcc.getcxxflags(cfg)
    local shared = config.mapFlags(cfg, wasmcc.shared)
    local cxxflags = config.mapFlags(cfg, wasmcc.cxxflags)
    local warnings = wasmcc.getwarnings(cfg)
    local sysversion = wasmcc.getsystemversionflags(cfg)
    return table.join(shared, cxxflags, warnings, sysversion)
end

function wasmcc.getwarnings(cfg)
    local flags = clang.getwarnings(cfg)
    return flags
end

function wasmcc.getsystemversionflags(cfg)
    local flags = clang.getsystemversionflags(cfg)
    return flags
end

function wasmcc.getdefines(defines)
    local flags = clang.getdefines(defines)
    table.insert(flags, "-D__WASM__")
    -- table.insert(flags, "-D_LIBCPP_ABI_VERSION=2")
    return flags
end

function wasmcc.getundefines(undefines)
    local flags = clang.getundefines(undefines)
    return flags
end

function wasmcc.getforceincludes(cfg)
    local flags = clang.getforceincludes(cfg)
    return flags
end

function wasmcc.getincludedirs(cfg, dirs)
    local flags = clang.getincludedirs(cfg, dirs)
    return flags
end

function wasmcc.getldflags(cfg)
    local flags = clang.getldflags(cfg)
    table.insert(flags, "-fuse-ld=wasm-ld")
    table.insert(flags, "-Wl,--export-dynamic")
    table.insert(flags, "-Wl,--fatal-warnings")
    table.insert(flags, "-Wl,--import-memory")
    -- table.insert(flags, "-Wl,--import-undefined")
    table.insert(flags, "-Wl,--no-entry")
    -- table.insert(flags, "-Wl,--unresolved-symbols=report-all")
    return flags
end

function wasmcc.getLibraryDirectories(cfg)
    local flags = clang.getLibraryDirectories(cfg)
    return flags
end

function wasmcc.getlinks(cfg, systemOnly)
    local flags = clang.getlinks(cfg, systemOnly)
    return flags
end

function wasmcc.getmakesettings(cfg)
    local flags = clang.getmakesettings(cfg)
    return flags
end

function wasmcc.gettoolname(cfg, tool)
    local name = clang.gettoolname(cfg, tool)
    return name
end

premake.tools["wasmcc"] = wasmcc

--
-- Private helper functions
--

function wasmcc._wasm_opt_args(prj, cfg)
    local shared_flags = wasmcc.getcppflags(cfg)
    local lang_flags = {}

    if prj.language == "C" then
        lang_flags = wasmcc.getcflags(cfg)
    elseif prj.language == "C++" then
        lang_flags = wasmcc.getcxxflags(cfg)
    else
        p.error("Unknown language '" .. cfg.language .. "'. Cannot generate wasmcc args.")
    end

    local flags = table.join(shared_flags, lang_flags, cfg.buildoptions)

    if not table.contains(flags, "--legalize-js-interface") then
        table.insert(flags, "--legalize-js-interface")
    end

    return table.concat(flags, " ")
end

function wasmcc._get_wasm_opt_path(cfg)
    local wasm_opt_path = os.getenv("WASM_OPT_PATH")

    if wasm_opt_path == nil then
        wasm_opt_path = cfg.wasmoptpath
    end

    if wasm_opt_path == nil then
        wasm_opt_path = "wasm-opt"
    end

    if os.ishost("windows") and not wasm_opt_path:match(".exe$") then
        wasm_opt_path = wasm_opt_path .. ".exe"
    end

    return wasm_opt_path
end
