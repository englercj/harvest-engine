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

-- We want to treat these keys as valid, but ignore them when applying module keys.
-- Most of these are handled explicitly, or are private, so we don't want to warn
-- about missing handlers for them.
local module_keys_ignore = {
    "_plugin",
    "conditions",
    "name",
    "type",
    "group",
}

-- Variants are not allowed to override these keys.
local variant_keys_disallow = {
    "name",
    "type",
    "variants",
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

local imported_plugins = {}
local imported_plugins_count = 0

local imported_modules = {}
local imported_modules_count = 0

local key_handlers = {}

local function _try_handle_key(ctx, key, value)
    if key == nil or value == nil then
        return
    end

    if table.contains(module_keys_ignore, key) then
        return
    end

    local handler = key_handlers[key]
    if handler == nil then
        p.warn("Module '" .. ctx.name .. "' contains unknown key '" .. key .. "'.")
    else
        handler(ctx, value)
    end
end

key_handlers.files = function (ctx, values)
    files(values)
end

key_handlers.warnings = function (ctx, value)
    warnings(value)
end

key_handlers.post_build_commands = function (ctx, values)
    postbuildcommands(values)
end

key_handlers.pre_build_commands = function (ctx, values)
    prebuildcommands(values)
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

key_handlers.exec = function (ctx, value)
    local func = dofile(value)
    func(ctx)
end

key_handlers.variants = function (ctx, values)
    for _, variant in ipairs(values) do
        if variant.conditions == nil then
            filter { }
        else
            filter(variant.conditions)
        end

        for key, value in orderedPairs(variant) do
            if table.contains(variant_keys_disallow, key) then
                p.warn("Module '" .. ctx.name .. "' has a variant (index " .. _ .. ") that tries to override a disallowed key '" .. key .. "'.")
            else
                _try_handle_key(ctx, key, value)
            end
        end

        filter { }
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

        if mod.warnings == nil or mod.warnings == "Extra" then
            filter { "toolset:msc-*" }
                buildoptions {
                    "/we4668", -- A symbol that was not defined was used with a preprocessor directive.
                }
        end

        _platform_file_excludes()

    os.chdir(oldcwd)
end

local function _should_include_module(mod, options)
    if options.exclude_groups and table.contains(options.exclude_groups, mod.group) then
        verbosef("Skipping module '%s' because the group '%s' is excluded.", mod.name, mod.group)
        return false
    end

    if options.include_groups and not table.contains(options.include_groups, mod.group) then
        verbosef("Skipping module '%s' because the group '%s' is not included.", mod.name, mod.group)
        return false
    end

    return true
end

local function _import_plugin(plugin_path, options)
    -- Search for the plugin json file
    local plugin_file = plugin_path

    if not path.hasextension(plugin_file, ".json") then
        plugin_file = path.join(plugin_file, "he_plugin.json")
    end

    plugin_file = path.getabsolute(plugin_file);

    if not os.isfile(plugin_file) then
        verbosef("Skipping import of plugin '%s', no file exists at: %s", plugin_path, plugin_file)
        return nil
    end

    -- Load the plugin file
    verbosef("Loading plugin imported as '%s' from file: %s", plugin_path, plugin_file)
    local plugin = json.decode(io.readfile(plugin_file))

    if plugin == nil then
        p.error("Load of plugin file failed. Is the JSON valid? " .. plugin_file)
        return nil
    end

    local oldcwd = os.getcwd()
    os.chdir(path.getdirectory(plugin_file))

    -- Mark the plugin as imported and install
    assert(type(plugin.id) == "string", "Plugins that provide modules must specify an 'id' key.")

    imported_plugins[plugin.id] = plugin
    imported_plugins_count = imported_plugins_count + 1

    plugin._file_path = plugin_file

    if install_plugin(plugin) == false then
        return {}
    end

    -- Check if the plugin imports additional plugins, and if so import them first
    local imports_other_plugins = false

    if plugin.import_plugins ~= nil then
        imports_other_plugins = true
        assert(type(plugin.import_plugins) == "table", "Plugin at '" .. plugin_path .. "' incorrectly specifies the 'import_plugins' key. It must be an array of string paths.")
        import_plugins(plugin.import_plugins, options)
    end

    -- Check if the plugin provides any modules, and warn if it doesn't.
    if plugin.modules == nil or table.isempty(plugin.modules) then
        verbosef("No modules listed in plugin '%s'", plugin.id)
        if not imports_other_plugins then
            p.warn("Plugin '" .. plugin.name .. "' imported from '" .. plugin_path .. "' contains no modules, and imports no plugins.")
        end
        return {}
    end

    local imported = {}

    for _, mod in ipairs(plugin.modules) do
        verbosef("Examining plugin module '%s'...", mod.name)
        mod._plugin = plugin

        local existing = imported_modules[mod.name]
        assert(existing == nil, "Module '" .. mod.name .. "' was already provided by plugin '" .. (existing and existing._plugin.id or "") .. "', but plugin '" .. plugin.id .. "' also provides it.")

        if _should_include_module(mod, options) then
            imported_modules[mod.name] = mod
            imported_modules_count = imported_modules_count + 1

            table.insert(imported, mod);
        end
    end

    os.chdir(oldcwd)

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

local function _register_module_key(dst, key, handler)
    if dst == nil then
        assert(key_handlers[key] == nil, "There is already a handler registered for: " .. key)
        key_handlers[key] = handler
    else
        local public_key = "public_" .. key
        local private_key = "private_" .. key

        assert(key_handlers[public_key] == nil, "There is already a handler registered for: " .. public_key)
        assert(key_handlers[private_key] == nil, "There is already a handler registered for: " .. private_key)

        table.insert(dst, { public_key, public_key })

        key_handlers[public_key] = handler
        key_handlers[private_key] = handler
    end
end

local function _get_scope_table(scope)
    if scope == "include" then
        return module_dependency_include_keys
    elseif scope == "link" then
        return module_dependency_link_keys
    elseif scope == "private" then
        return nil
    end

    p.error("Cannot add module keys to unknown scope: '" .. scope .. "'")
end

function add_module_key(scope, key, handler)
    if handler == nil then
        handler = function (ctx, value)
            _G[k](value)
        end
    end

    local dst = _get_scope_table(scope)
    _register_module_key(dst, key, handler);
end

function add_module_keys(scope, keys)
    assert(type(keys) == "table", "add_module_keys expects a table of keys")

    for _, k in ipairs(keys) do
        if type(k) == "string" then
            add_module_key(scope, k)
        else
            add_module_key(scope, k[1], k[2])
        end
    end
end

function get_module(name)
    return imported_modules[name]
end

function get_all_modules()
    return imported_modules
end

function get_plugin(id)
    return imported_plugins[id]
end

function get_all_plugins()
    return imported_plugins
end

function handle_module_key(mod, key, value)
    _try_handle_key(mod, key, value)
end
