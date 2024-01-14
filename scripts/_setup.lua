-- Copyright Chad Engler

he = {}

-- Core utilities come first, they are used everywhere
include "utils.lua"

-- Foundational functionality for the rest of the system
include "options.lua"
include "globals.lua"
include "platforms.lua"
include "modules.lua"
include "workspace.lua"
include "builtin_module_keys.lua"

-- Add custom actions
include "actions/docs.lua"

-- Add custom systems
include "systems/wasm.lua"
