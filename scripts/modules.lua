-- Copyright Chad Engler

local p = premake
local install_plugin = dofile("install_plugin.lua")
local build_type = "dynamic" -- TODO: Switch based on dynamic or static build of modules

local target_dir_by_kind = {
    ConsoleApp = target_bin_dir,
    WindowedApp = target_bin_dir,
    SharedLib = target_bin_dir,
    StaticLib = target_lib_dir,
}

local kind_by_module_type = {
    default = (build_type == "dynamic" and "SharedLib" or "StaticLib"),
    static = "StaticLib",
    shared = "SharedLib",
    header = "StaticLib",
    test = "StaticLib",
    console_app = "ConsoleApp",
    windowed_app = "WindowedApp",
}

local variant_keys_disallow = {
    "name",
    "type",
    "variants",
    "condition",
}

local module_dependency_include_keys = {
    -- { handler key, value key }
    { "public_defines", "public_defines" },
    { "public_dependson_include", "public_dependson" }, -- Treat dependson like it is an include dependency
    { "public_dependson_include", "public_dependson_include" },
    { "public_includedirs", "public_includedirs" },
}

local module_dependency_link_keys = {
    -- { handler key, value key }
    { "public_dependson", "public_dependson" },
}

local imported_modules = {}
local imported_modules_count = 0
local key_handlers = {}

local function _try_handle_key(ctx, key, value)
    if key == nil or value == nil then
        return
    end

    local handler = key_handlers[key]
    if handler ~= nil then
        handler(ctx, value)
    end
end

key_handlers.files = function (ctx, values)
    files(values)
end

key_handlers.public_defines = function (ctx, values)
    defines(values)
end
key_handlers.private_defines = key_handlers.public_defines

key_handlers.public_includedirs = function (ctx, values)
    includedirs(values)
end
key_handlers.private_includedirs = key_handlers.public_includedirs

key_handlers.public_dependson = function (ctx, values)
    for _, mod_name in ipairs(values) do
        if string.startswith(mod_name, "sys:") then
            links { string.sub(mod_name, 5) }
            return
        end

        if string.startswith(mod_name, "file:") then
            links { string.sub(mod_name, 6) }
            return
        end

        if string.startswith(mod_name, "module:") then
            mod_name = string.sub(mod_name, 8)
        end

        local mod = imported_modules[mod_name]
        assert(mod ~= nil, "Module '" .. ctx.name .. "' has a dependency on '" .. mod_name .. "', but no such module has been imported.")

        if ctx.type == "console_app" or ctx.type == "windowed_app" or (ctx.type == "default" and build_type == "dynamic") then
            if mod.type == "static" or mod.type == "test" then
                links { mod.name }
            end
        end

        local oldcwd = os.getcwd()
        os.chdir(mod._plugin._install_dir)

        for _, keys in ipairs(module_dependency_include_keys) do
            _try_handle_key(ctx, keys[1], mod[keys[2]])
        end

        for _, keys in ipairs(module_dependency_link_keys) do
            _try_handle_key(ctx, keys[1], mod[keys[2]])
        end

        os.chdir(oldcwd)
    end
end
key_handlers.private_dependson = key_handlers.public_dependson

key_handlers.public_dependson_include = function (ctx, values)
    for _, mod_name in ipairs(values) do
        if string.startswith(mod_name, "sys:") or string.startswith(mod_name, "file:") then
            return
        end

        if string.startswith(mod_name, "module:") then
            mod_name = string.sub(mod_name, 8)
        end

        local mod = imported_modules[mod_name]
        assert(mod ~= nil, "Module '" .. ctx.name .. "' has an include dependency on '" .. mod_name .. "', but no such module has been imported.")

        local oldcwd = os.getcwd()
        os.chdir(mod._plugin._install_dir)

        for _, keys in ipairs(module_dependency_include_keys) do
            _try_handle_key(ctx, keys[1], mod[keys[2]])
        end

        os.chdir(oldcwd)
    end
end
key_handlers.private_dependson_include = key_handlers.public_dependson_include

key_handlers.exec = function (ctx, values)
    for _, file_path in ipairs(values) do
        local func = dofile(file_path)
        func(ctx)
    end
end

local function _platform_file_excludes()
    filter { "files:**.emscripten.*" }  flags { "ExcludeFromBuild" }
    filter { "files:**.linux.*" }       flags { "ExcludeFromBuild" }
    filter { "files:**.posix.*" }       flags { "ExcludeFromBuild" }
    filter { "files:**.win32.*" }       flags { "ExcludeFromBuild" }
    filter { "files:**.xbox.*" }        flags { "ExcludeFromBuild" }

    filter { "system:emscripten", "files:**.emscripten.*" }
        removeflags { "ExcludeFromBuild" }

    filter { "system:linux", "files:**.linux.*" }
        removeflags { "ExcludeFromBuild" }

    filter { "system:posix", "files:**.posix.*" }
        removeflags { "ExcludeFromBuild" }

    filter { "system:windows or xbox", "files:**.win32.*" }
        removeflags { "ExcludeFromBuild" }

    filter { "system:xbox", "files:**.xbox.*" }
        removeflags { "ExcludeFromBuild" }

    filter { }
end


local function _module_project(mod)
    verbosef("Generating project for module '%s' in group '%s'", mod.name, mod.group)

    local oldcwd = os.getcwd()
    os.chdir(mod._plugin._install_dir)

    if type(mod.type) ~= "string" or mod.type == "" then
        mod.type = "default"
    end

    local kindname = kind_by_module_type[mod.type]
    assert(kindname, "Unknown module type: '" .. mod.type .. "'.")

    local kind_target_dir = target_dir_by_kind[kindname]
    assert(kind_target_dir, "No target_dir known for kind: '" .. kindname .. "'.")

    group(mod.group)
    project(mod.name)
        language "C++"
        kind(kindname)
        objdir(target_obj_dir)
        targetdir(kind_target_dir)
        location(projects_dir)

        defines { "HE_CFG_MODULE_NAME=\"" .. mod.name .. "\"" }

        if kindname == "SharedLib" then
            defines { "HE_CFG_MODULE_SHARED=1" }
        elseif kindname == "StaticLib" then
            defines { "HE_CFG_MODULE_STATIC=1" }
        end

        for key, value in orderedPairs(mod) do
            _try_handle_key(mod, key, value)
        end

        _platform_file_excludes()

    os.chdir(oldcwd)
end

local function _should_include_module(mod, options)
    if options.exclude_groups and table.contains(options.exclude_groups, mod.group) then
        return false
    end

    if options.include_groups and not table.contains(options.include_groups, mod.group) then
        return false
    end

    return true
end

local function _is_variant_active(condition)
    if condition == nil then
        return true
    end

    -- Check `system`
    if condition.system ~= nil then
        local sysTags = os.getSystemTags(os.target())
        if not table.contains(sysTags, condition.system) then
            return false
        end
    end

    return true
end

local function _process_variants(mod)
    if mod.variants == nil then
        return
    end

    for _, variant in ipairs(mod.variants) do
        if _is_variant_active(variant.condition) then
            for key, value in orderedPairs(variant) do
                if not table.contains(variant_keys_disallow, key) then
                    local t = type(mod[key])

                    assert(t == "nil" or t == type(value), "Mismatched types for variant key '" .. key .. "' in module '" .. mod.name .. "'.")

                    -- TODO: "remove_*" keys that subtract from the module

                    if t == "table" then
                        mod[key] = table.join(mod[key], value)
                    else
                        mod[key] = value
                    end
                end
            end
        end
    end
end

local function _import_plugin(plugin_path, options)
    local plugin_file = plugin_path

    if not path.hasextension(plugin_file, ".json") then
        plugin_file = path.join(plugin_file, "he_plugin.json")
    end

    plugin_file = path.getabsolute(plugin_file);

    verbosef("Loading plugin imported as '%s' from file '%s'", plugin_path, plugin_file)
    local plugin = json.decode(io.readfile(plugin_file))

    if plugin == nil then
        return nil
    end

    local warn_about_no_modules = true

    if plugin.import_plugins ~= nil then
        warn_about_no_modules = false
        assert(type(plugin.import_plugins) == "table", "Plugin at '" .. plugin_path .. "' incorrectly specifies the 'import_plugins' key. It must be an array of string paths.")

        local oldcwd = os.getcwd()
        os.chdir(path.getdirectory(plugin_file))

        import_plugins(plugin.import_plugins, options)

        os.chdir(oldcwd)
    end

    if plugin.modules == nil or table.isempty(plugin.modules) then
        if warn_about_no_modules then
            p.warn("Plugin '" .. plugin.name .. "' imported from '" .. plugin_path .. "' contains no modules.")
        end
        return {}
    end

    plugin._file_path = plugin_file
    install_plugin(plugin)

    local imported = {}

    for _, mod in ipairs(plugin.modules) do
        mod._plugin = plugin

        local existing = imported_modules[mod.name]
        assert(existing == nil, "Module '" .. mod.name .. "' was already provided by plugin '" .. (existing and existing._plugin.id or "") .. "', but plugin '" .. plugin.id .. "' also provides it.")

        if _should_include_module(mod, options) then
            _process_variants(mod)

            imported_modules[mod.name] = mod
            imported_modules_count = imported_modules_count + 1

            table.insert(imported, mod);
        end
    end

    return imported
end

function import_plugins(plugins, options)
    if type(plugins) == "string" then
        plugins = { plugins }
    end

    assert(type(plugins) == "table", "import_plugins expects a string or a table of strings")

    if options == nil then
        options = {}
    end

    local imported = {}

    for _, plugin_path in ipairs(plugins) do
        local dirs = os.matchdirs(plugin_path)
        local import_count = 0

        if table.isempty(dirs) then
            dirs = { plugin_path }
        end

        for _, plugin_dir in ipairs(dirs) do
            local imported_mods = _import_plugin(plugin_dir, options)

            if imported_mods ~= nil then
                import_count = import_count + 1

                for _, mod in ipairs(imported_mods) do
                    table.insert(imported, mod);
                end
            end
        end

        if import_count == 0 then
            p.warn("No plugins found to import using path '" .. plugin_path .. "'.")
        end
    end

    for _, mod in ipairs(imported) do
        _module_project(mod)
    end
end

local function _register_module_key(key, handler, dstScope)
    if dstScope == nil then
        assert(key_handlers[key] == nil, "There is already a handler registered for: " .. key)
        key_handlers[key] = handler
    else
        local public_key = "public_" .. key
        local private_key = "private_" .. key

        assert(key_handlers[public_key] == nil, "There is already a handler registered for: " .. public_key)
        assert(key_handlers[private_key] == nil, "There is already a handler registered for: " .. private_key)

        table.insert(dstScope, { public_key, public_key })

        key_handlers[public_key] = handler
        key_handlers[private_key] = handler
    end
end

function add_module_keys(scope, keys)
    assert(type(keys) == "table", "add_module_keys expects a table of keys")

    local dst = nil
    if scope == "include" then
        dst = module_dependency_include_keys
    elseif scope == "link" then
        dst = module_dependency_link_keys
    elseif scope == "private" then
        -- Private doesn't propagate the values anywhere
    else
        p.error("Cannot add module keys to unknown scope: '" .. scope .. "'")
        return
    end

    for _, k in ipairs(keys) do
        if type(k) == "string" then
            -- Handle the simple string input where we will call a global function of the same name
            local handler = function (ctx, value)
                _G[key](value)
            end

            _register_module_key(k, handler, dst);
        else
            -- Handle the object input where the handler is explicit
            local key = k[1]
            local handler = k[2]

            assert(type(key) == "string" and type(handler) == "function", "Elements of a module key extension are expected to be an array where the first element is the string key and the second is the handler. E.g.: { \"key\", handler }")
            _register_module_key(k, handler, dst);
        end
    end
end

function get_module(name)
    return imported_modules[name]
end

function get_all_modules()
    return imported_modules
end

function handle_module_key(mod, key, value)
    _try_handle_key(mod, key, value)
end
