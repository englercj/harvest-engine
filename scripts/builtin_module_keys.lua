-- Copyright Chad Engler

local p = premake

local function _handle_variant_key(ctx, key, value)
    if table.contains(he.variant_disallow_keys, key) then
        p.warn("Module '" .. ctx.name .. "' has a variant (index " .. _ .. ") that tries to override a disallowed key '" .. key .. "'.")
    else
        he.try_handle_module_key(ctx, key, value)
    end
end

local function _handle_variants(ctx, values, key_list)
    for _, variant in ipairs(values) do
        he.filter_push(variant.conditions)

        if key_list == nil then
            for key, value in he.ordered_pairs(variant) do
                _handle_variant_key(ctx, key, value)
            end
        else
            for _, key in ipairs(key_list) do
                _handle_variant_key(ctx, key, variant[key])
            end
        end

        he.filter_pop()
    end
end

local function _handle_dependson_include(ctx, values)
    for _, mod_name in ipairs(values) do
        if string.startswith(mod_name, "sys:") or string.startswith(mod_name, "file:") then
            return
        end

        if string.startswith(mod_name, "module:") then
            mod_name = string.sub(mod_name, 8)
        end

        local mod = he.imported_modules[mod_name]
        assert(mod ~= nil, "Module '" .. ctx.name .. "' has an include dependency on '" .. mod_name .. "', but no such module has been imported.")

        if mod._plugin._install_valid == false then
            verbosef("Module '%s' has an include dependency on '%s' but it was not installed, ignoring.", ctx.name, mod_name)
            return
        end

        local oldcwd = os.getcwd()
        os.chdir(mod._plugin._install_dir)

        for _, key in ipairs(he.module_dependency_include_keys) do
            he.try_handle_module_key(ctx, key, mod[key])
        end

        -- Propagate the `public_dependson` key as if it was the include variant
        he.try_handle_module_key(ctx, "public_dependson_include", mod.public_dependson)

        if mod.variants ~= nil then
            _handle_variants(ctx, mod.variants, he.module_dependency_include_keys)
        end

        os.chdir(oldcwd)
    end
end

local function _handle_dependson(ctx, values)
    for _, mod_name in ipairs(values) do
        if string.startswith(mod_name, "sys:") then
            if ctx.type == "console_app" or ctx.type == "windowed_app" or (ctx.type == "default" and not he.is_static_only) then
                links { string.sub(mod_name, 5) }
            end
            return
        end

        if string.startswith(mod_name, "file:") then
            if ctx.type == "console_app" or ctx.type == "windowed_app" or (ctx.type == "default" and not he.is_static_only) then
                links { string.sub(mod_name, 6) }
            end
            return
        end

        if string.startswith(mod_name, "module:") then
            mod_name = string.sub(mod_name, 8)
        end

        local mod = he.imported_modules[mod_name]
        assert(mod ~= nil, "Module '" .. ctx.name .. "' has a dependency on '" .. mod_name .. "', but no such module has been imported.")

        if mod._plugin._install_valid == false then
            verbosef("Module '%s' has a dependency on '%s' but it was not installed, ignoring.", ctx.name, mod_name)
            return
        end

        if ctx.type == "console_app" or ctx.type == "windowed_app" or (ctx.type == "default" and not he.is_static_only) then
            if mod.type == "default" or mod.type == "static" then
                links { mod.name }
            elseif mod.type == "custom" then
                dependson { mod.name }
            end
        end

        local oldcwd = os.getcwd()
        os.chdir(mod._plugin._install_dir)

        for _, key in ipairs(he.module_dependency_include_keys) do
            he.try_handle_module_key(ctx, key, mod[key])
        end

        -- Propagate the `public_dependson` key as if it was the include variant
        he.try_handle_module_key(ctx, "public_dependson_include", mod.public_dependson)

        for _, key in ipairs(he.module_dependency_link_keys) do
            he.try_handle_module_key(ctx, key, mod[key])
        end

        if mod.variants ~= nil then
            _handle_variants(ctx, mod.variants, he.module_dependency_include_keys)
            _handle_variants(ctx, mod.variants, he.module_dependency_link_keys)
        end

        os.chdir(oldcwd)
    end
end

he.add_module_key {
    key = "defines",
    scope = "include",
    type = "table",
    desc = "an array of strings",
    handler = function (ctx, values) defines(values) end,
}
he.add_module_key {
    key = "disablewarnings",
    scope = "private",
    type = "table",
    desc = "an array of warning specified to be disabled",
    handler = function (ctx, values) disablewarnings(values) end,
}

he.add_module_key {
    key = "includedirs",
    scope = "include",
    type = "table",
    desc = "an array of string include paths (relative to the he_plugin file)",
    handler = function (ctx, values) includedirs(values) end,
}

he.add_module_key {
    key = "externalincludedirs",
    scope = "include",
    type = "table",
    desc = "an array of string include paths (relative to the he_plugin file)",
    handler = function (ctx, values) externalincludedirs(values) end,
}

he.add_module_key {
    key = "buildoptions",
    scope = "include",
    type = "table",
    desc = "an array of string build options",
    handler = function (ctx, values) buildoptions(values) end,
}

he.add_module_key {
    key = "dependson_include",
    scope = "include",
    type = "table",
    desc = "an array of module names",
    handler = function (ctx, values) _handle_dependson_include(ctx, values) end,
}

he.add_module_key {
    key = "dependson",
    scope = "link",
    type = "table",
    desc = "an array of module names, system libraries ('sys:X'), or files ('file:X')",
    handler = function (ctx, values) _handle_dependson(ctx, values) end,
}

he.add_module_key {
    key = "targetname",
    scope = "private",
    type = "string",
    desc = "string name of the output file (without the extension)",
    handler = function (ctx, value) targetname(value) end,
}

he.add_module_key {
    key = "files",
    scope = "private",
    type = "table",
    desc = "a table of strings",
    handler = function (ctx, values) files(values) end,
}

he.add_module_key {
    key = "warnings",
    scope = "private",
    type = "string",
    desc = "one of: 'off', 'default', or 'extra'",
    handler = function (ctx, value) warnings(value) end,
}

he.add_module_key {
    key = "post_build_commands",
    scope = "private",
    type = "table",
    desc = "an array of strings",
    handler = function (ctx, values) postbuildcommands(values) end,
}

he.add_module_key {
    key = "pre_build_commands",
    scope = "private",
    type = "table",
    desc = "an array of strings",
    handler = function (ctx, values) prebuildcommands(values) end,
}

he.add_module_key {
    key = "exec",
    scope = "private",
    type = "string",
    desc = "a string file path",
    handler = function (ctx, value)
        local func = include(value)
        func(ctx)
    end,
}

he.add_module_key {
    key = "variants",
    scope = "private",
    type = "table",
    desc = "an array of variant objects",
    handler = function (ctx, values) _handle_variants(ctx, values) end,
}
