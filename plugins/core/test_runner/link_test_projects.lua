return function (ctx)
    local imported_modules = get_all_modules()

    for name, mod in orderedPairs(imported_modules) do
        if mod.group == "tests" and mod.type == "static" then
            handle_module_key(ctx, "public_dependson", { mod.name })

            -- TODO: remove this hack for the test_runner, needed right now because the "unused"
            -- test symbols are otherwise stripped out.
            filter { "toolset:msc-*", "language:C++" }
                linkoptions { "/WHOLEARCHIVE:" .. mod.name }
            filter { "toolset:gcc or clang" }
                linkoptions { "-Wl,--whole-archive %{path.getdirectory(target_lib_dir)}/" .. mod.name .. "/lib" .. mod.name .. ".a -Wl,--no-whole-archive" }
            filter { }
        end
    end
end
