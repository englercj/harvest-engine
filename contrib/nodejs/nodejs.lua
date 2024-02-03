-- Copyright Chad Engler

return function (plugin, install_dirs)
    function he.get_nodejs_path()
        local install_dir = install_dirs["*"]
        local node_path = path.join(install_dir, "node")

        if os.ishost("windows") then
            node_path = node_path .. ".exe"
        end

        return node_path
    end

    function he.get_npmjs_path()
        local install_dir = install_dirs["*"]
        local npm_path = path.join(install_dir, "npm")

        if os.ishost("windows") then
            npm_path = npm_path .. ".cmd"
        end

        return npm_path
    end
end
