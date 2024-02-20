-- Copyright Chad Engler

local p = premake
local api = p.api
local config = p.config

--
-- Register the WASM extensions
--

api.addAllowed("system", { "wasm" })
api.addAllowed("architecture", { "wasm32", "wasm64" })
api.addAllowed("toolset", { "wasmcc" })
api.addAllowed("vectorextensions", { "SIMD128" })

local os_option = p.option.get("os")
if os_option ~= nil then
    table.insert(os_option.allowed, { "wasm" })
end

os.systemTags["wasm"] = { "wasm", "web" }

-- Path to the wasm-opt executable used for optimizing the final binary.
-- Can also specify this in the environment variable "WASM_OPT_PATH".
api.register {
    name = "wasmoptpath",
    scope = "config",
    kind = "path",
    tokens = true,
}

-- Individual features to enable for the wasmcc toolset.
api.register {
    name = "wasmfeatures",
    scope = "config",
    kind = "list:string",
    allowed = {
        "RelaxedSIMD",
        "NontrappingFPToInt",
        "SignExt",
        "ExceptionHandling",
        "BulkMemory",
        "Atomics",
        "MutableGlobals",
        "Multivalue",
        "TailCall",
        "ReferenceTypes",
        "ExtendedConst",
        "MultiMemory",
    },
}

--
-- Setup some sane defaults for WASM system
--

filter { "system:wasm" }
    architecture("wasm32")
    toolset("wasmcc")

filter { "system:wasm", "kind:StaticLib or SharedLib" }
    targetextension ".bc"

filter { "system:wasm", "kind:ConsoleApp or WindowedApp" }
    targetextension ".wasm"

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
        wasm32 = "-target wasm32 -D__WASM32__",
        wasm64 = "-target wasm64 -D__WASM64__",
    },
    wasmfeatures = {
        RelaxedSIMD = "-mrelaxed-simd",
        NontrappingFPToInt = "-mnontrapping-fptoint",
        SignExt = "-msign-ext",
        ExceptionHandling = "-mexception-handling",
        BulkMemory = "-mbulk-memory",
        Atomics = "-matomics",
        MutableGlobals = "-mmutable-globals",
        Multivalue = "-mmultivalue",
        TailCall = "-mtail-call",
        ReferenceTypes = "-mreference-types",
        ExtendedConst = "-mextended-const",
        MultiMemory = "-mmultimemory",
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

function wasmcc.getincludedirs(cfg, dirs, extdirs, frameworkdirs, includedirsafter)
    local flags = clang.getincludedirs(cfg, dirs, extdirs, frameworkdirs, includedirsafter)
    return flags
end

function wasmcc.getsharedlibarg(cfg)
    local flags = clang.getsharedlibarg(cfg)
    return flags
end

function wasmcc.getldflags(cfg)
    local flags = clang.getldflags(cfg)
    table.insert(flags, "-fuse-ld=wasm-ld")
    return flags
end

function wasmcc.getLibraryDirectories(cfg)
    local flags = clang.getLibraryDirectories(cfg)
    return flags
end

function wasmcc.getlinks(cfg, systemonly, nogroups)
    local flags = clang.getlinks(cfg, systemonly, nogroups)
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

--
-- Harvest extensions
--

local function _can_enable_wasm_platform()
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

he.add_module_key {
    key = "wasmfeatures",
    scope = "include",
    type = "table",
    desc = "an array of wasm features to enable",
    handler = function (ctx, values) wasmfeatures(values) end,
}

he.add_platform {
    name = "Wasm32",
    hosts = { "windows", "linux" },
    actions = { "gmake2" },
    system = "wasm",
    architecture = "wasm32",
    on_define = function ()
        vectorextensions "SIMD128"
        defines { "HE_PLATFORM_WASM" }
    end,
    can_enable = _can_enable_wasm_platform,
}

-- No browsers with wasm64 support in their public versions yet

-- he.add_platform {
--     name = "Wasm64",
--     hosts = { "windows", "linux" },
--     system = "wasm",
--     architecture = "wasm64",
--     on_define = function ()
--         vectorextensions "SIMD128"
--         defines { "HE_PLATFORM_WASM" }
--     end,
--     can_enable = _can_enable_wasm_platform,
-- }
