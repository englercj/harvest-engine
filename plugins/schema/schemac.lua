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

                if options.includeDirs then
                    for _, dir in ipairs(options.includeDirs) do
                        opt = opt .. "-I " .. dir .. " "
                    end
                end

                local buildCmd = he.target_bin_dir .. "/he_schemac " .. opt .. "-o " .. he.file_gen_dir .. " %{file.abspath}"

                dependson { "he_schemac" }

                he.filter_push_combine { "files:" .. options.glob }
                    compilebuildoutputs "on"
                    buildmessage "Compiling schema file %{file.abspath}"
                    buildcommands {
                        "{ECHO} " .. buildCmd,
                        buildCmd
                    }
                    buildoutputs {
                        he.file_gen_dir .. "/%{file.basename}.hsc.h",
                        he.file_gen_dir .. "/%{file.basename}.hsc.cpp",
                    }

                he.filter_pop()
            end
        end
    }
end
