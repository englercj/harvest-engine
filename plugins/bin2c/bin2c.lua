-- Copyright Chad Engler

local function _get_bin2c_proj_name(mod_name, unique_id)
    return mod_name .. "__bin2c_" .. unique_id
end

local function _get_next_unique_id(ctx)
    ctx._bin2c_compiler_index = (ctx._bin2c_compiler_index or 0) + 1
    return ctx._bin2c_compiler_index
end

return function (plugin)
    he.add_module_key {
        key = "bin2c_compile",
        scope = "private",
        type = "table",
        desc = "an array of compilation target configuration objects",
        prepare = function (ctx, values)
            for _, options in ipairs(values) do
                local unique_id = _get_next_unique_id(ctx)
                local project_name = _get_bin2c_proj_name(ctx.name, unique_id)
                assert(ctx._plugin.modules[project_name] == nil, "Module '" .. ctx.name .. "' seems to have been defined twice.")

                verbosef("Creating bin2c generation module: %s", project_name)

                -- Create a new bin2c generation module
                local includedirs = {}
                for _, dir in ipairs(options.includedirs or {}) do
                    table.insert(includedirs, "%{he.get_generated_dir('" .. project_name .. "')}/" .. dir)
                end

                local scope = options.scope or "public"
                local ctx_dep_key = scope .. "_dependson"

                local new_module = {
                    name = project_name,
                    type = "custom",
                    group = options.group or "_generated/bin2c",
                    _requires_build_order_dependency = true,
                }
                if he.filter_get_active() ~= nil then
                    new_module.variants = {
                        {
                            conditions = he.filter_get_active(),
                            files = options.files or {},
                            public_includedirs = includedirs,
                            bin2c_compile_internal = options,
                        },
                    }
                    ctx.variants = ctx.variants or {}
                    table.insert(ctx.variants, {
                        conditions = he.filter_get_active(),
                        [ctx_dep_key] = { project_name },
                    })
                else
                    new_module.files = options.files or {}
                    new_module.public_includedirs = includedirs
                    new_module.bin2c_compile_internal = options

                    ctx[ctx_dep_key] = ctx[ctx_dep_key] or {}
                    table.insert(ctx[ctx_dep_key], project_name)
                end
                ctx._plugin.modules[project_name] = new_module
                he.add_module(ctx._plugin, new_module)
            end
        end,
    }

    he.add_module_key {
        key = "bin2c_compile_internal",
        scope = "private",
        type = "table",
        desc = "a table of compilation target configuration",
        handler = function (ctx, options)
            local exe = he.target_bin_dir .. "/he_bin2c" .. iif(os.istarget("win32"), ".exe", "")
            local args = {exe}

            if options.text then
                table.insert(args, "-t")
            end

            if options.compress then
                table.insert(args, "-c")
            end

            for _, dir in ipairs(options.public_includedirs or {}) do
                table.insert(args, "-I")
                table.insert(args, dir)
            end

            -- variable name
            table.insert(args, "-n")
            table.insert(args, "c_%{file.name:gsub('[%.-]', '_')}")

            -- input file
            table.insert(args, "-f")
            table.insert(args, "\"%{file.abspath}\"")

            -- output file
            table.insert(args, "-o")
            table.insert(args, he.file_gen_dir .. "/%{file.name}.h")

            local cmd = table.concat(args, " ")

            dependson { "he_bin2c" }

            for _, file_path in ipairs(options.files) do
                he.filter_push_combine { "files:" .. file_path }
                    compilebuildoutputs "off"
                    buildmessage "Creating C header for file %{file.abspath}"
                    buildcommands { "{ECHO} " .. cmd, cmd }
                    buildinputs { exe }
                    buildoutputs { he.file_gen_dir .. "/%{file.name}.h" }
                he.filter_pop()
            end
        end,
    }
end
