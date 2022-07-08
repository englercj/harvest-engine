return function (ctx)
    for name, mod in he.ordered_pairs(he.imported_modules) do
        if mod.group == "engine/tests" and mod.type == "static" then
            he.try_handle_module_key(ctx, "public_dependson", { mod.name })

            -- Prevent link-time stripping of symbols from the library being tested.
            -- Without this test cases are removed because they are not referenced anywhere.
            filter { "toolset:msc-*", "language:C++" }
                linkoptions { "/WHOLEARCHIVE:" .. mod.name }
            filter { "toolset:gcc or clang" }
                linkoptions { "-Wl,--whole-archive %{path.getdirectory(he.target_lib_dir)}/" .. mod.name .. "/lib" .. mod.name .. ".a -Wl,--no-whole-archive" }
            filter { }
        end
    end
end
