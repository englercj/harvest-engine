-- Copyright Chad Engler

local function _get_shaderc_proj_name(mod_name, unique_id)
    return mod_name .. "__shaderc_" .. unique_id
end

local function _get_next_unique_id(ctx)
    ctx._shader_compiler_index = (ctx._shader_compiler_index or 0) + 1
    return ctx._shader_compiler_index
end

return function (plugin)
    he.add_module_key {
        key = "shader_compile",
        scope = "private",
        type = "table",
        desc = "an array of compilation target configuration objects",
        prepare = function (ctx, values)
            for _, options in ipairs(values) do
                local unique_id = _get_next_unique_id(ctx)
                local project_name = _get_shaderc_proj_name(ctx.name, unique_id)
                assert(ctx._plugin.modules[project_name] == nil, "Module '" .. ctx.name .. "' seems to have been defined twice.")

                verbosef("Creating shader generation module: %s", project_name)

                -- Create a new shader generation module
                local includedirs = {}
                for _, dir in ipairs(options.includedirs or {}) do
                    table.insert(includedirs, "%{he.get_generated_dir('" .. project_name .. "')}/" .. dir)
                end

                local scope = options.scope or "public"
                local ctx_dep_key = scope .. "_dependson"

                local new_module = {
                    name = project_name,
                    type = "custom",
                    group = options.group or "_generated/shader",
                    _requires_build_order_dependency = true,
                }
                if he.filter_get_active() ~= nil then
                    new_module.variants = {
                        {
                            conditions = he.filter_get_active(),
                            files = options.files or {},
                            public_includedirs = includedirs,
                            shader_compile_internal = options,
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
                    new_module.shader_compile_internal = options

                    ctx[ctx_dep_key] = ctx[ctx_dep_key] or {}
                    table.insert(ctx[ctx_dep_key], project_name)
                end
                ctx._plugin.modules[project_name] = new_module
                he.add_module(ctx._plugin, new_module)
            end
        end,
    }

    he.add_module_key {
        key = "shader_compile_internal",
        scope = "private",
        type = "table",
        desc = "a table of compilation target configuration",
        handler = function (ctx, options)
            local exe = he.target_bin_dir .. "/he_shaderc" .. iif(os.istarget("win32"), ".exe", "")
            local args = {exe}

            for _, target in ipairs(options.targets or {}) do
                table.insert(args, "-t")
                table.insert(args, target)
            end

            for _, dir in ipairs(options.includedirs or {}) do
                local install_dirs = ctx._plugin._install_dirs
                for system, install_dir in pairs(install_dirs) do
                    table.insert(args, "-I")
                    table.insert(args, "\"" .. path.join(install_dir, dir) .. "\"")
                end
            end

            for _, def in ipairs(options.defines or {}) do
                table.insert(args, "-O")
                table.insert(args, def)
            end

            if options.optimize then
                table.insert(args, "-O")
                table.insert(args, options.optimize)
            end

            -- output directory
            table.insert(args, "-o")
            table.insert(args, he.get_file_gen_dir(ctx.name))

            -- input file
            table.insert(args, "\"%{file.abspath}\"")

            local cmd = table.concat(args, " ")

            dependson { "he_shaderc" }

            for _, file_path in ipairs(options.files) do
                he.filter_push_combine { "files:" .. file_path }
                    compilebuildoutputs "off"
                    buildmessage "Compiling shader file %{file.abspath}"
                    buildcommands { "{ECHO} " .. cmd, cmd }
                    buildinputs { exe }
                    buildoutputs { he.get_file_gen_dir(ctx.name) .. "/%{file.basename}.shaders.h" }
                he.filter_pop()
            end
        end,
    }
end
