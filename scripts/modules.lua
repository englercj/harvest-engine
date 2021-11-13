-- Copyright Chad Engler

local p = premake
local TOML = dofile("toml.lua")
local install_plugin = dofile("install_plugin.lua")

local target_dir_by_kind = {
    ConsoleApp = he.target_bin_dir,
    WindowedApp = he.target_bin_dir,
    SharedLib = he.target_bin_dir,
    StaticLib = he.target_lib_dir,
}

local kind_by_module_type = {
    default = (he.is_static_only and "StaticLib" or "SharedLib"),
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
local module_ignore_keys = {
    "_plugin",
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

local function _try_handle_key(ctx, key, value)
    if key == nil or value == nil then
        return
    end

    if table.contains(module_ignore_keys, key) then
        return
    end

    local info = he.module_key_infos[key]
    if info == nil then
        p.warn("Module '" .. ctx.name .. "' contains unknown key '" .. key .. "'.")
    else
        assert(type(value) == info.type, "Unexpected type for '" .. key .. "' in module '" .. ctx.name .. "', expected " .. info.desc)
        info.handler(ctx, value)
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
    if mod._plugin._install_valid == false then
        verbosef("Skipping project for module '%s' in group '%s', not installed.", mod.name, mod.group)
	return
    end

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
        location(he.projects_dir)

        defines { "HE_CFG_MODULE_NAME=\"" .. mod.name .. "\"" }

        if kindname == "SharedLib" then
            defines { "HE_CFG_MODULE_SHARED=1" }
        elseif kindname == "StaticLib" then
            defines { "HE_CFG_MODULE_STATIC=1" }
        end

        for key, value in he.ordered_pairs(mod) do
            _try_handle_key(mod, key, value)
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
    -- Search for the plugin toml file
    local plugin_file = plugin_path

    if not path.hasextension(plugin_file, ".toml") then
        plugin_file = path.join(plugin_file, "he_plugin.toml")
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

    local oldcwd = os.getcwd()
    os.chdir(path.getdirectory(plugin_file))

    -- Mark the plugin as imported and install
    assert(type(plugin.id) == "string", "Plugins that provide modules must specify an 'id' key.")

    he.imported_plugins[plugin.id] = plugin
    he.imported_plugins_count = he.imported_plugins_count + 1

    plugin._file_path = plugin_file
    plugin._install_valid, plugin._install_dir = install_plugin(plugin)

    -- Check if the plugin imports additional plugins, and if so import them first
    local imports_other_plugins = false

    if plugin.import_plugins ~= nil then
        imports_other_plugins = true
        assert(type(plugin.import_plugins) == "table", "Plugin at '" .. plugin_path .. "' incorrectly specifies the 'import_plugins' key. It must be an array of string paths.")
        he.import_plugins(plugin.import_plugins, options)
    end

    -- Check if the plugin provides any modules, and warn if it doesn't.
    if plugin.modules == nil or table.isempty(plugin.modules) then
        verbosef("No modules listed in plugin '%s'", plugin.id)
        if not imports_other_plugins then
            p.warn("Plugin '" .. plugin.name .. "' imported from '" .. plugin_path .. "' contains no modules, and imports no plugins.")
        end
        os.chdir(oldcwd)
        return {}
    end

    local imported = {}

    for _, mod in ipairs(plugin.modules) do
        verbosef("Examining plugin module '%s'...", mod.name)
        mod._plugin = plugin

        local existing = he.imported_modules[mod.name]
        assert(existing == nil, "Module '" .. mod.name .. "' was already provided by plugin '" .. (existing and existing._plugin.id or "") .. "', but plugin '" .. plugin.id .. "' also provides it.")

        if _should_include_module(mod, options) then
            he.imported_modules[mod.name] = mod
            he.imported_modules_count = he.imported_modules_count + 1

            table.insert(imported, mod);
        end
    end

    os.chdir(oldcwd)
    return imported
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
    assert(type(plugins) == "table", "import_plugins expects a table of plugins to import")

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

he.add_module_key = function (info)
    local dst = _get_scope_table(info.scope)

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

he.get_module = function (name)
    return he.imported_modules[name]
end

he.get_plugin = function (id)
    return he.imported_plugins[id]
end

he.try_handle_module_key = function (mod, key, value)
    _try_handle_key(mod, key, value)
end
