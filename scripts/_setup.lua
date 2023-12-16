-- Copyright Chad Engler

he = {}

include "utils.lua"

include "options.lua"
include "globals.lua"
include "platforms.lua"
include "modules.lua"
include "workspace.lua"

include "builtin_module_keys.lua"

include "emscripten/_preload.lua"
include "emscripten/emscripten.lua"

include "docgen/_preload.lua"
include "docgen/docgen.lua"
