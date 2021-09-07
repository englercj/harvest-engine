-- Copyright Chad Engler

return function (plugin)
    he.add_module_key {
        key = "bin2c_compile",
        scope = "private",
        type = "table",
        desc = "an array of string path globs to compile",
        handler = function (ctx, values)
            for _, options in ipairs(values) do
                local opt = ""

                if options.text then
                    opt = opt .. "-t "
                end
                if options.compress then
                    opt = opt .. "-c "
                end

                dependson { "he_bin2c" }

                filter { "files:" .. options.glob }
                    compilebuildoutputs "on"
                    buildmessage "Creating C header for file %{file.abspath}"
                    buildcommands {
                        he.target_bin_dir .. "/he_bin2c " .. opt .. "-n c_%{file.name:gsub('[%.-]', '_')} -f %{file.abspath} -o " .. he.file_gen_dir .. "/%{file.name}.h",
                    }
                    buildoutputs {
                        he.file_gen_dir .. "/%{file.name}.h",
                    }

                filter { }
            end
        end
    }
end
