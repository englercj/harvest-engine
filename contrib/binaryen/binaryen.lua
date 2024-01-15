-- Copyright Chad Engler

return function (plugin, install_dirs)
    local install_dir = install_dirs["*"]
    assert(install_dir ~= nil, "No install directory found for plugin: " .. plugin.id)

    local ext = iif(os.ishost("windows"), ".exe", "")
    wasmoptpath(path.join(install_dir, "bin", "wasm-opt" .. ext))
end
