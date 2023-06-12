-- Copyright Chad Engler

return function (plugin)
    he.add_module_key {
        key = "bin2c_compile",
        scope = "private",
        type = "table",
        desc = "an array of compilation target configuration objects",
        handler = function (ctx, values)
            for _, options in ipairs(values) do
                local opt = ""

                if options.text then
                    opt = opt .. "-t "
                end
                if options.compress then
                    opt = opt .. "-c "
                end

                local exe = he.target_bin_dir .. "/he_bin2c" .. iif(os.istarget("win32"), ".exe", "")
                local buildCmd = exe .. " " .. opt .. "-n c_%{file.name:gsub('[%.-]', '_')} -f \"%{file.abspath}\" -o " .. he.file_gen_dir .. "/%{file.name}.h"

                files(options.files)
                dependson { "he_bin2c" }

                for _, file_path in ipairs(options.files) do
                    he.filter_push_combine { "files:" .. file_path }
                        compilebuildoutputs "off"
                        buildmessage "Creating C header for file %{file.abspath}"
                        buildcommands { "{ECHO} " .. buildCmd, buildCmd }
                        buildinputs { exe }
                        buildoutputs { he.file_gen_dir .. "/%{file.name}.h" }
                    he.filter_pop()
                end
            end
        end
    }
end
