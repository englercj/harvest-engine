-- Copyright Chad Engler

return function (plugin)
    local npm_path = he.get_npmjs_path()

    he.cwd_push("test_app/wasm")
    os.executef("%s install --silent", npm_path)
    he.cwd_pop()
end
