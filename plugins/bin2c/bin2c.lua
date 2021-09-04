-- Copyright Chad Engler

return function (plugin)
    add_module_key("private", "bin2c_compile", function (ctx, values)
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
                    target_bin_dir .. "/he_bin2c " .. opt .. "-n c_%{file.name:gsub('[%.-]', '_')} -f %{file.abspath} -o " .. file_gen_dir .. "/%{file.name}.h",
                }
                buildoutputs {
                    file_gen_dir .. "/%{file.name}.h",
                }

            filter { }
        end
    end)
end
