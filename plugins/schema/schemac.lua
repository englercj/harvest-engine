-- Copyright Chad Engler

return function (plugin)
    he.add_module_key {
        key = "schema_compile",
        scope = "private",
        type = "table",
        desc = "an array of compilation target configuration objects",
        handler = function (ctx, values)
            for _, options in ipairs(values) do
                local opt = ""

                if options.targets then
                    for _, target in ipairs(options.targets) do
                        opt = opt .. "-t " .. target .. " "
                    end
                end

                if options.includedirs then
                    for _, dir in ipairs(options.includedirs) do
                        opt = opt .. "-I \"" .. dir .. "\" "
                    end
                end

                if options.dependson_include then
                    for _, mod_name in ipairs(options.dependson_include) do
                        local mod = he.get_module(mod_name)
                        assert(mod ~= nil, "Module '" .. ctx.name .. "' has a he_schema dependency on '" .. mod_name .. "', but no such module has been imported.")
                        if mod._plugin._install_valid == false then
                            verbosef("Module '%s' has a he_schema dependency on '%s' but it was not installed, ignoring.", ctx.name, mod_name)
                        else
                            if mod.public_includedirs then
                                -- If we can include a file from this module, then we actually have a hard dependency on that module.
                                -- This is because we only need include access to build the schema files, but the generated
                                -- cpp files will require the generated files from `mod` to be able to compile correctly.
                                dependson { mod.name }
                                for _, dir in ipairs(mod.public_includedirs) do
                                    opt = opt .. "-I \"" .. path.join(mod._plugin._install_dir, dir) .. "\" "
                                end
                            end
                        end
                    end
                end

                local exe = he.target_bin_dir .. "/he_schemac" .. iif(os.istarget("win32"), ".exe", "")
                local buildCmd = exe .. " " .. opt .. "-o " .. he.file_gen_dir .. " \"%{file.abspath}\""

                files(options.files)
                dependson { "he_schemac" }

                for _, file_path in ipairs(options.files) do
                    he.filter_push_combine { "files:" .. file_path }
                        compilebuildoutputs "on"
                        buildmessage "Compiling schema file %{file.abspath}"
                        buildcommands { "{ECHO} " .. buildCmd, buildCmd }
                        buildinputs { exe }
                        buildoutputs {
                            he.file_gen_dir .. "/%{file.basename}.hsc.h",
                            he.file_gen_dir .. "/%{file.basename}.hsc.cpp",
                        }
                    he.filter_pop()
                end
            end
        end
    }
end
