-- Copyright Chad Engler

return function (plugin, install_dirs)
    function he.get_wasm_opt_path()
        local install_dir = install_dirs["*"]
        local wasm_opt_path = path.join(install_dir, "bin", "wasm-opt")

        if os.ishost("windows") then
            wasm_opt_path = wasm_opt_path .. ".exe"
        end

        return wasm_opt_path
    end
end
