--
-- Name:        emscripten/_preload.lua
-- Purpose:     Define the Emscripten APIs
-- Author:      Chad Engler
-- Copyright:   (c) 2021 Chad Engler
--

local p = premake
local api = p.api

premake.tools.emcc = {}

--
-- Register the Emscripten extension
--

p.EMSCRIPTEN = "emscripten"
p.EMCC = "emcc"

api.addAllowed("system", { p.EMSCRIPTEN })
api.addAllowed("flags", {
    "EmCpuProfiler",
    "EmMemoryProfiler",
    "EmThreadProfiler",
    "EmIgnoreDynamicLinking",
    "EmSIMD",
    "EmSSE",
})

local osoption = p.option.get("os")
if osoption ~= nil then
    table.insert(osoption.allowed, { p.EMSCRIPTEN })
end

os.systemTags[p.EMSCRIPTEN] = { p.EMSCRIPTEN, "posix" }

--
-- Register Emscripten properties
--

api.register {
    name = "emccpath",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "em_linkeroptimize",
    scope = "config",
    kind = "string",
    allowed = {
        "Off",
        "Simple",
        "On",
        "Unsafe",
    },
}

-- Sets any settings.js value. The full list can be found here:
-- https://github.com/emscripten-core/emscripten/blob/master/src/settings.js
api.register {
    name = "em_options",
    scope = "config",
    kind = "list:string",
    tokens = true,
}

api.register {
    name = "em_library",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "em_prepend",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "em_append",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "em_sourcemapbase",
    scope = "config",
    kind = "string",
    tokens = true,
}

api.register {
    name = "em_embedfile",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "em_preloadfile",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "em_htmlshell",
    scope = "config",
    kind = "path",
    tokens = true,
}

api.register {
    name = "em_exportedfunctions",
    scope = "config",
    kind = "list:string",
    tokens = true,
}

--
-- Setup some sane defaults for various emscripten projects
--

filter { "system:emscripten" }
    architecture "x86"
    toolset(p.EMCC)

filter { "system:emscripten", "kind:StaticLib" }
    targetextension ".bc"

filter { "system:emscripten", "kind:SharedLib" }
    targetextension ".bc"

filter { "system:emscripten", "kind:ConsoleApp" }
    targetextension ".js"

filter { "system:emscripten", "kind:WindowedApp" }
    targetextension ".html"

filter {}

--
-- Register emscripten module keys so they can be used in module json descriptors.
--

he.add_module_key {
    key = "em_options",
    scope = "include",
    type = "table",
    desc = "",
    handler = function (ctx, values) em_options(values) end,
}

he.add_module_key {
    key = "em_linkeroptimize",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_linkeroptimize(value) end,
}

he.add_module_key {
    key = "em_library",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_library(value) end,
}

he.add_module_key {
    key = "em_prepend",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_prepend(value) end,
}

he.add_module_key {
    key = "em_append",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_append(value) end,
}

he.add_module_key {
    key = "em_sourcemapbase",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_sourcemapbase(value) end,
}

he.add_module_key {
    key = "em_embedfile",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_embedfile(value) end,
}

he.add_module_key {
    key = "em_preloadfile",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_preloadfile(value) end,
}

he.add_module_key {
    key = "em_htmlshell",
    scope = "link",
    type = "string",
    desc = "",
    handler = function (ctx, value) em_htmlshell(value) end,
}

he.add_module_key {
    key = "em_exportedfunctions",
    scope = "link",
    type = "table",
    desc = "",
    handler = function (ctx, values) em_exportedfunctions(values) end,
}

--
-- Decide when the full module should be loaded.
--

return function(cfg)
    return cfg.system == p.EMSCRIPTEN or cfg.toolset == p.EMCC
end
