-- Copyright Chad Engler

return function (plugin)
    function he.get_nodejs_path()
        local node_path = path.join(he.get_plugin("nodejs.nodejs")._install_dirs["*"], "node")

        if os.ishost("windows") then
            node_path = node_path .. ".exe"
        end

        return node_path
    end

    function he.get_npmjs_path()
        local npm_path = path.join(he.get_plugin("nodejs.nodejs")._install_dirs["*"], "npm")

        if os.ishost("windows") then
            npm_path = npm_path .. ".cmd"
        end

        return npm_path
    end
end
