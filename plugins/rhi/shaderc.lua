-- Copyright Chad Engler

return function (plugin)
    he.add_module_key {
        key = "shader_compile",
        scope = "private",
        type = "table",
        desc = "an array of compilation target configuration objects",
        handler = function (ctx, values)
            for _, options in ipairs(values) do
                local opt = ""

                if options.optimize then
                    opt = opt .. "-O " .. options.optimize .. " "
                end

                if options.defines then
                    for _, def in ipairs(options.defines) do
                        opt = opt .. "-D " .. def .. " "
                    end
                end

                if options.include_dirs then
                    for _, dir in ipairs(options.include_dirs) do
                        opt = opt .. "-I \"" .. dir .. "\" "
                    end
                end

                if options.targets then
                    for _, target in ipairs(options.targets) do
                        opt = opt .. "-t " .. target .. " "
                    end
                end

                local exe = he.target_bin_dir .. "/he_shaderc" .. iif(os.istarget("win32"), ".exe", "")
                local buildCmd = exe .. " " .. opt .. "-o " .. he.file_gen_dir .. " \"%{file.abspath}\""

                files(options.files)
                dependson { "he_shaderc" }

                for _, file_path in ipairs(options.files) do
                    he.filter_push_combine { "files:" .. file_path }
                        compilebuildoutputs "off"
                        buildmessage "Compiling shader file %{file.abspath}"
                        buildcommands { "{ECHO} " .. buildCmd, buildCmd }
                        buildinputs { exe }
                        buildoutputs { he.file_gen_dir .. "/%{file.basename}.shaders.h" }
                    he.filter_pop()
                end
            end
        end
    }
end
