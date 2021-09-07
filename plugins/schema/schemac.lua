-- Copyright Chad Engler

return function (plugin)
    he.add_module_key {
        key = "schema_compile",
        scope = "private",
        type = "table",
        desc = "an array of string path globs to compile",
        handler = function (ctx, values)
            for _, options in ipairs(values) do
                local opt = ""

                -- TODO: Includedirs
                -- TODO: grpc

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
