-- Copyright Chad Engler

local p = premake
local TOML = dofile("toml.lua")
local install_plugin = dofile("install_plugin.lua")

local target_dir_by_kind = {
    ConsoleApp = he.target_bin_dir,
    WindowedApp = he.target_bin_dir,
    SharedLib = he.target_bin_dir,
    StaticLib = he.target_lib_dir,
    Utility = he.target_lib_dir,
}

local module_type_by_kind = {
    ConsoleApp = 1,
    WindowedApp = 2,
    SharedLib = 3,
    StaticLib = 4,
    Utility = 5,
}

local kind_by_module_type = {
    console_app = "ConsoleApp",
    custom = "Utility",
    header = "StaticLib",
    static = "StaticLib",
    shared = "SharedLib",
    windowed_app = "WindowedApp",
}

-- We want to treat these keys as valid, but ignore them when applying module keys.
-- Most of these are handled explicitly, or are private, so we don't want to warn
-- about missing handlers for them.
local module_ignore_keys = {
    "conditions",
    "name",
    "type",
    "group",
}

-- Variants are not allowed to override these keys.
he.variant_disallow_keys = {
    "name",
    "type",
    "variants",
}

he.module_dependency_include_keys = {}
he.module_dependency_link_keys = {}

he.imported_plugins = {}
he.imported_plugins_count = 0

he.imported_modules = {}
he.imported_modules_count = 0

he.module_key_infos = {}

he.module_dependency_handlers = {}

local function _try_run_module_key_handler(ctx, key, value, handler_name)
    if key == nil or value == nil then
        return
    end

    if key:sub(1, 1) == "_" or table.contains(module_ignore_keys, key) then
        return
    end

    local info = he.module_key_infos[key]
    if info == nil then
        p.warn("Module '" .. ctx.name .. "' contains unknown key '" .. key .. "'.")

        if he.module_key_infos["public_" .. key] ~= nil then
            p.warn("    Did you mean 'public_" .. key .. "' or 'private_" .. key .. "'?")
        end

        return
    end

    assert(type(value) == info.type, "Unexpected type for '" .. key .. "' in module '" .. ctx.name .. "', expected " .. info.desc)
    if info[handler_name] ~= nil then
        info[handler_name](ctx, value)
    end
end

local function _system_tag_file_excludes()
    local seen = {}

    for _, tag_list in he.ordered_pairs(os.systemTags) do
        for _, tag in ipairs(tag_list) do
            if not table.contains(seen, tag) then
                table.insert(seen, tag)

                local system_filter = "system:" .. tag
                local files_filter = "files:**." .. tag .. ".*"

                filter { files_filter }
                    flags { "ExcludeFromBuild" }

                filter { system_filter, files_filter }
                    removeflags { "ExcludeFromBuild" }
            end
        end
    end

    filter { }
end

local function _module_prepare(mod)
    if mod._plugin._install_valid == false then
        verbosef("Skipping prepare for module '%s' in group '%s', not installed.", mod.name, mod.group)
        return
    end

    verbosef("Running prepare for module '%s' in group '%s'", mod.name, mod.group)

    for key, value in he.ordered_pairs(mod) do
        he.try_prepare_module_key(mod, key, value)
    end
end

local function _module_project(mod)
    if mod._plugin._install_valid == false then
        verbosef("Skipping project for module '%s' in group '%s', not installed.", mod.name, mod.group)
        return
    end

    assert(type(mod.type) == "string", "Module type must be a string.")

    if mod.type == "content" then
        verbosef("Skipping project for module '%s' in group '%s', it is a content module.", mod.name, mod.group)
        return
    end

    verbosef("Generating project for module '%s' in group '%s'", mod.name, mod.group)

    local kindname = kind_by_module_type[mod.type]
    assert(kindname, "Unknown module type: '" .. mod.type .. "'.")

    local kind_target_dir = target_dir_by_kind[kindname]
    assert(kind_target_dir, "No target_dir known for kind: '" .. kindname .. "'.")

    local module_type = module_type_by_kind[kindname];
    local language_type = iif(mod.language ~= nil, mod.language, "C++")

    assert(mod._plugin._install_valid, "Cannot generated module project for '" .. mod.name .. "' because the plugin that provides it ('" .. mod._plugin.id .. "') is not installed.")
    local install_dirs = mod._plugin._install_dirs

    group(mod.group or "")
    project(mod.name)
        language(language_type)
        kind(kindname)
        objdir(he.target_obj_dir)
        targetdir(kind_target_dir)
        location(he.projects_dir)

        defines {
            "HE_CFG_MODULE_NAME=" .. mod.name,
            "HE_CFG_MODULE_TYPE=" .. module_type,
        }

        _system_tag_file_excludes()

        for system, install_dir in he.ordered_pairs(install_dirs) do
            he.cwd_push(install_dir)
            if system ~= "*" then
                printf("Pushing filter for system: %s in module: %s", system, mod.name)
                he.filter_push { "system:" .. system }
            end

            for key, value in he.ordered_pairs(mod) do
                he.try_handle_module_key(mod, key, value)
            end

            if system ~= "*" then
                he.filter_pop()
            end
            he.cwd_pop()
        end
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
    -- Search for the plugin toml file
    local plugin_file = plugin_path

    if not path.hasextension(plugin_file, ".toml") then
        plugin_file = path.join(plugin_path, "he_plugin.toml")
    end

    plugin_file = path.getabsolute(plugin_file);

    if not os.isfile(plugin_file) then
        verbosef("Skipping import of plugin '%s', no file exists at: %s", plugin_path, plugin_file)
        return nil
    end

    -- Load the plugin file
    verbosef("Loading plugin imported as '%s' from file: %s", plugin_path, plugin_file)
    local plugin, error = TOML.parse(io.readfile(plugin_file))

    if plugin == nil then
        p.error("Load of plugin file failed " .. plugin_file .. "\n" .. error)
        return nil
    end

    he.cwd_push(path.getdirectory(plugin_file))

    -- Mark the plugin as imported and install
    assert(type(plugin.id) == "string", "Plugins that provide modules must specify an 'id' key.")

    he.imported_plugins[plugin.id] = plugin
    he.imported_plugins_count = he.imported_plugins_count + 1

    plugin._file_path = plugin_file
    plugin._install_valid, plugin._install_dirs = install_plugin(plugin)

    -- Check if the plugin imports additional plugins, and if so import them first
    local imports_other_plugins = false

    if plugin.plugins ~= nil then
        imports_other_plugins = true
        assert(type(plugin.plugins) == "table", "Plugin at '" .. plugin_path .. "' incorrectly specifies the 'plugins' key. It must be an array of string paths.")
        he.import_plugins(plugin.plugins, options)
    end

    -- Check if the plugin provides any modules, and warn if it doesn't.
    if plugin.modules == nil or table.isempty(plugin.modules) then
        verbosef("No modules listed in plugin '%s'", plugin.id)
        he.cwd_pop()
        return {}
    end

    -- Import modules provided by this plugin
    for _, mod in ipairs(plugin.modules) do
        if _should_include_module(mod, options) then
            he.add_module(plugin, mod)
        end
    end

    -- Apply any extensions provided by this plugin
    if plugin.extend and plugin.extend.modules then
        for _, ext in ipairs(plugin.extend.modules) do
            local mod = he.imported_modules[ext.name]
            if not mod then
                premake.warn("No module named '" .. ext.name .. "' was found, but the plugin '" .. plugin.id .. "' tries to extend it.")
            else
                verbosef("Extending module '%s' with data from plugin '%s'...", mod.name, plugin.id)
                he.imported_modules[ext.name] = table.deep_merge(mod, ext)
            end
        end
    end

    he.cwd_pop()
end

local function _get_scope_table(scope)
    if scope == "include" then
        return he.module_dependency_include_keys
    elseif scope == "link" then
        return he.module_dependency_link_keys
    elseif scope == "private" then
        return nil
    end

    p.error("Cannot add module keys to unknown scope: '" .. scope .. "'")
end

he.import_plugins = function (plugins, options)
    assert(type(plugins) == "table", "he.import_plugins() expects a table of plugins to import")

    if options == nil then
        options = {}
    end

    for _, plugin_path in ipairs(plugins) do
        local dirs = os.matchdirs(plugin_path)
        if table.isempty(dirs) then
            dirs = { plugin_path }
        end

        for _, plugin_dir in ipairs(dirs) do
            _import_plugin(plugin_dir, options)
        end
    end
end

he.generate_module_projects = function ()
    for mod_name, mod in he.ordered_pairs(he.imported_modules) do
        _module_prepare(mod)
    end

    for mod_name, mod in he.ordered_pairs(he.imported_modules) do
        _module_project(mod)
    end
end

he.add_module_key = function (info)
    local dst = _get_scope_table(info.scope)

    -- dst is nil when the scope is 'private'
    if dst == nil then
        assert(he.module_key_infos[info.key] == nil, "There is already a module key registered with the name: " .. info.key)
        he.module_key_infos[info.key] = info

        if info.allow_in_variants == false then
            table.insert(he.variant_disallow_keys, info.key)
        end
    else
        local public_key = "public_" .. info.key
        local private_key = "private_" .. info.key

        assert(he.module_key_infos[public_key] == nil, "There is already a module key registered with the name: " .. public_key)
        assert(he.module_key_infos[private_key] == nil, "There is already a module key registered with the name: " .. private_key)

        table.insert(dst, public_key)

        he.module_key_infos[public_key] = info
        he.module_key_infos[private_key] = info
    end
end

he.add_module_dependency_handler = function (handler)
    table.insert(he.module_dependency_handlers, handler)
end

he.get_module = function (name)
    return he.imported_modules[name]
end

he.add_module = function (plugin, mod)
    verbosef("Importing plugin module '%s'...", mod.name)

    mod._plugin = plugin

    local existing = he.imported_modules[mod.name]
    assert(existing == nil, "Module '" .. mod.name .. "' was already provided by plugin '" .. (existing and existing._plugin.id or "") .. "', but plugin '" .. plugin.id .. "' also provides it.")

    he.imported_modules[mod.name] = mod
    he.imported_modules_count = he.imported_modules_count + 1
end

he.get_plugin = function (id)
    return he.imported_plugins[id]
end

he.try_handle_module_key = function (mod, key, value)
    return _try_run_module_key_handler(mod, key, value, "handler")
end

he.try_prepare_module_key = function (mod, key, value)
    return _try_run_module_key_handler(mod, key, value, "prepare")
end
