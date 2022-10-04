-- Copyright Chad Engler

local p = premake

p.modules.emscripten = {}
local m = p.modules.emscripten

m._VERSION = p._VERSION

include("emscripten_emcc.lua")

return m
