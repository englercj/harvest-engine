-- Copyright Chad Engler

return function (plugin)
    he.add_module_key {
        key = "shader_compile",
        scope = "private",
        type = "table",
        desc = "an array of string path globs to compile",
        handler = function (ctx, values)
            for _, options in ipairs(values) do
                local opt = ""

                if options.targets then
                    for _, target in ipairs(options.targets) do
                        opt = opt .. "-t " .. target .. " "
                    end
                end

                dependson { "he_shaderc" }

                filter { "files:" .. options.glob }
                    compilebuildoutputs "off"
                    buildmessage "Compiling shader file %{file.abspath}"
                    buildcommands {
                        he.target_bin_dir .. "/he_shaderc %{file.abspath} " .. opt .. "-o " .. he.file_gen_dir,
                    }
                    buildoutputs {
                        he.file_gen_dir .. "/%{file.name}_generated.h",
                    }

                filter { }
            end
        end
    }
end
