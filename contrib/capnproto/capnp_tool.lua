-- Copyright Chad Engler

return function (plugin)
    he.add_module_key {
        key = "capnp_compile",
        scope = "private",
        type = "table",
        desc = "an array of compilation target configuration objects",
        handler = function (ctx, values)
            for _, options in ipairs(values) do
                local opt = ""

                if options.targets then
                    for _, target in ipairs(options.targets) do
                        opt = opt .. "-o" .. he.target_bin_dir .. "/capnpc-" .. target .. ":" .. he.file_gen_dir .. " "
                        -- opt = opt .. "-o" .. target .. ":" .. he.file_gen_dir .. " "
                    end
                end

                if options.includeDirs then
                    for _, dir in ipairs(options.includeDirs) do
                        opt = opt .. "-I" .. dir .. " "
                    end
                end

                opt = opt .. "--src-prefix=%{file.directory}"

                dependson { "capnp_tool", "capnpc-c++", "capnpc-capnp" }

                he.filter_push_combine { "files:" .. options.glob }
                    compilebuildoutputs "on"
                    buildmessage "Compiling capnp file %{file.abspath}"
                    buildcommands {
                        he.target_bin_dir .. "/capnp compile " .. opt .. " %{file.abspath}",
                    }
                    buildoutputs {
                        he.file_gen_dir .. "/%{file.basename}.capnp.c++",
                        he.file_gen_dir .. "/%{file.basename}.capnp.h",
                    }

                he.filter_pop()
            end
        end
    }
end
