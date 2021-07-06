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
-- Update the package scrips to know how to propagate emscripten keys
--

add_module_keys("include", {
    "em_options",
})
add_module_keys("link", {
    "em_linkeroptimize",
    "em_options",
    "em_library",
    "em_prepend",
    "em_append",
    "em_sourcemapbase",
    "em_embedfile",
    "em_preloadfile",
    "em_htmlshell",
    "em_exportedfunctions",
})

--
-- Decide when the full module should be loaded.
--

return function(cfg)
    return cfg.system == p.EMSCRIPTEN or cfg.toolset == p.EMCC
end
