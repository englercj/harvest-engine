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

                if options.rpc then
                    opt = opt .. "--grpc "
                end

                if options.includeDirs then
                    for _, dir in ipairs(options.includeDirs) do
                        opt = opt .. "-I " .. dir .. " "
                    end
                end

                dependson { "he_schemac" }

                filter { "files:" .. options.glob }
                    compilebuildoutputs "on"
                    buildmessage "Compiling schema file %{file.abspath}"
                    buildcommands {
                        he.target_bin_dir .. "/he_schemac -o " .. he.file_gen_dir .. " %{file.abspath}",
                    }
                    buildoutputs {
                        he.file_gen_dir .. "/%{file.name}_generated.h",
                    }

                filter { }
            end
        end
    }
end
