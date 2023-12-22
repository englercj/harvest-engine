-- Copyright Chad Engler

local function _get_schemac_proj_name(mod_name, unique_id)
    return mod_name .. "__schemac_" .. unique_id
end

local function _get_next_unique_id(ctx)
    ctx._schema_compiler_index = (ctx._schema_compiler_index or 0) + 1
    return ctx._schema_compiler_index
end

local function _collect_schema_include_args(ctx, args, dep_name)
    local dep_mod = he.get_module(dep_name)
    assert(dep_mod ~= nil, "Module '" .. ctx.name .. "' has a schema dependency on '" .. dep_name .. "', but no such module has been imported.")
    assert(dep_mod.schema_compile ~= nil, "Module '" .. ctx.name .. "' has a schema dependency on '" .. dep_name .. "', but that module exposes no schema files.")

    if dep_mod._plugin._install_valid == false then
        verbosef("Module '%s' has a schema dependency on '%s' but it was not installed, ignoring.", ctx.name, mod_name)
        return
    end

    for _, options in ipairs(dep_mod.schema_compile) do
        for _, dir in ipairs(options.includedirs or {}) do
            local install_dirs = dep_mod._plugin._install_dirs
            for system, install_dir in pairs(install_dirs) do
                table.insert(args, "-I")
                table.insert(args, "\"" .. path.join(install_dir, dir) .. "\"")
            end
        end

        for _, mod_name in ipairs(options.dependson or {}) do
            _collect_schema_include_args(dep_mod, args, mod_name)
        end
    end
end

return function (plugin)
    he.add_module_key {
        key = "schema_compile",
        scope = "private",
        type = "table",
        desc = "an array of compilation target configuration objects",
        prepare = function (ctx, values)
            for _, options in ipairs(values) do
                local unique_id = _get_next_unique_id(ctx)
                local project_name = _get_schemac_proj_name(ctx.name, unique_id)
                assert(ctx._plugin.modules[project_name] == nil, "Module '" .. ctx.name .. "' seems to have been defined twice.")

                verbosef("Creating schema generation module: %s", project_name)

                -- Create a new schema generation module
                local includedirs = {}
                for _, dir in ipairs(options.includedirs or {}) do
                    table.insert(includedirs, "%{he.get_generated_dir('" .. project_name .. "')}/" .. dir)
                end

                local deps = {"he_core", "he_schema"}
                local variants = {}
                for _, dep_name in ipairs(options.dependson or {}) do
                    local dep_mod = he.get_module(dep_name)
                    assert(dep_mod ~= nil, "Module '" .. ctx.name .. "' has a schema dependency on '" .. dep_name .. "', but no such module has been imported.")
                    assert(dep_mod.schema_compile ~= nil, "Module '" .. ctx.name .. "' has a schema dependency on '" .. dep_name .. "', but that module exposes no schema files.")

                    local dep_unique_id = 0
                    for _, _ in ipairs(dep_mod.schema_compile) do
                        dep_unique_id = dep_unique_id + 1
                        table.insert(deps, _get_schemac_proj_name(dep_name, dep_unique_id))
                    end

                    for _, variant in ipairs(dep_mod.variants or {}) do
                        if variant.schema_compile ~= nil then
                            local new_variant = {
                                conditions = variant.conditions,
                                public_dependson = {},
                            }
                            for _, _ in ipairs(variant.schema_compile) do
                                dep_unique_id = dep_unique_id + 1
                                table.insert(new_variant.public_dependson, _get_schemac_proj_name(dep_name, dep_unique_id))
                            end
                            table.insert(variants, new_variant)
                        end
                    end
                end

                local scope = options.scope or "public"
                local ctx_dep_key = scope .. "_dependson"

                local new_module = {
                    name = project_name,
                    type = "static",
                    group = options.group or "_generated/schema",
                    variants = variants,
                    _requires_build_order_dependency = true,
                }
                if he.filter_get_active() ~= nil then
                    table.insert(new_module.variants, {
                        conditions = he.filter_get_active(),
                        files = options.files or {},
                        public_includedirs = includedirs,
                        public_dependson = deps,
                        schema_compile_internal = options,
                    })
                    ctx.variants = ctx.variants or {}
                    table.insert(ctx.variants, {
                        conditions = he.filter_get_active(),
                        [ctx_dep_key] = { project_name },
                    })
                else
                    new_module.files = options.files or {}
                    new_module.public_includedirs = includedirs
                    new_module.public_dependson = deps
                    new_module.schema_compile_internal = options

                    ctx[ctx_dep_key] = ctx[ctx_dep_key] or {}
                    table.insert(ctx[ctx_dep_key], project_name)
                end
                ctx._plugin.modules[project_name] = new_module
                he.add_module(ctx._plugin, new_module)
            end
        end,
    }

    he.add_module_key {
        key = "schema_compile_internal",
        scope = "private",
        type = "table",
        desc = "a table of compilation target configuration",
        handler = function (ctx, options)
            local exe = he.target_bin_dir .. "/he_schemac" .. iif(os.ishost("windows"), ".exe", "")
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

            for _, mod_name in ipairs(options.dependson or {}) do
                _collect_schema_include_args(ctx, args, mod_name)
            end

            -- Always add the he_schema include directory
            local schema_mod = he.get_module("he_schema")
            for _, dir in ipairs(schema_mod.public_includedirs or {}) do
                local install_dirs = schema_mod._plugin._install_dirs
                for system, install_dir in pairs(install_dirs) do
                    table.insert(args, "-I")
                    table.insert(args, "\"" .. path.join(install_dir, dir) .. "\"")
                end
            end

            -- output directory
            table.insert(args, "-o")
            table.insert(args, he.get_file_gen_dir(ctx.name))

            -- input file
            table.insert(args, "\"%{file.abspath}\"")

            local cmd = table.concat(args, " ")

            dependson { "he_schemac" }

            for _, file_path in ipairs(options.files) do
                he.filter_push_combine { "files:" .. file_path }
                    compilebuildoutputs "on"
                    buildmessage "Compiling schema file %{file.abspath}"
                    buildcommands { "{ECHO} " .. cmd, cmd }
                    buildinputs { exe }
                    buildoutputs {
                        he.get_file_gen_dir(ctx.name) .. "/%{file.basename}.hsc.h",
                        he.get_file_gen_dir(ctx.name) .. "/%{file.basename}.hsc.cpp",
                    }
                he.filter_pop()
            end
        end,
    }
end
