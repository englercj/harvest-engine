-- Copyright Chad Engler

return function (plugin, install_dirs)
    -- Just pull the first one, they should all be the same for emscripten
    local install_dir = install_dirs["*"]
    assert(install_dir ~= nil, "No install directory found for plugin: " .. plugin.id)

    print("EMSCRIPTEN PATH: " .. path.join(install_dir, "system", "lib"))
    wasmlibpath(path.join(install_dir, "system", "lib"))
end
