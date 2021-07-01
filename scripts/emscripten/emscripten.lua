-- Copyright Chad Engler

local p = premake

if not p.modules.emscripten then
    p.modules.emscripten = {}
    p.modules.emscripten._VERSION = p._VERSION

    include('emscripten_emcc.lua')
end

return p.modules.emscripten
