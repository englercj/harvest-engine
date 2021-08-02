-- Copyright Chad Engler

local install_plugin = dofile("install_plugin.lua")
local build_type = "dynamic" -- TODO: Switch based on dynamic or static build of modules

local target_dir_by_kind = {
    ConsoleApp = bin_dir,
    WindowedApp = bin_dir,
    SharedLib = bin_dir,
    StaticLib = lib_dir,
}

local kind_by_module_type = {
    default = (build_type == "dynamic" and "SharedLib" or "StaticLib"),
    static = "StaticLib",
    header = "StaticLib",
    test = "StaticLib",
    console_app = "ConsoleApp",
    windowed_app = "WindowedApp",
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
        if string.starts_with(mod_name, "system:") then
            links { string.sub(mod_name, 8) }
            return
        end

        if string.starts_with(mod_name, "file:") then
            links { string.sub(mod_name, 6) }
            return
        end

        if string.starts_with(mod_name, "module:") then
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
        local mod = imported_modules[mod_name]
        assert(mod ~= nil, "Module '" .. ctx.name .. "' has an include dependency on '" .. mod_name .. "', but no such module has been imported.")

        if string.starts_with(mod_name, "system:") or string.starts_with(mod_name, "file:") then
            return
        end

        if string.starts_with(mod_name, "module:") then
            mod_name = string.sub(mod_name, 8)
        end

        local oldcwd = os.getcwd()
        os.chdir(mod._plugin._install_dir)

        for _, keys in ipairs(module_dependency_include_keys) do
            _try_handle_key(ctx, keys[1], mod[keys[2]])
        end

        os.chdir(oldcwd)
    end
end
key_handlers.private_dependson_include = key_handlers.public_dependson_include

key_handlers.variants = function (ctx, values)
    for _, variant in ipairs(values) do
        if variant.filters ~= nil then
            filter(variant.filters)
        end

        for key, value in orderedPairs(variant) do
            _try_handle_key(mod, key, value)
        end
    end

    filter { }
end

key_handlers.link_all_module_tests = function (ctx, value)
    if value ~= true then
        return
    end

    for name, mod in orderedPairs(imported_modules) do
        if mod.type == "test" then
            key_handlers.public_dependson(ctx, { mod.name })

            filter { "toolset:msc-*", "language:C++" }
                linkoptions { "/WHOLEARCHIVE:" .. mod.name }
            filter { "toolset:gcc or clang" }
                linkoptions { "-Wl,--whole-archive %{lib_base_dir}/" .. mod.name .. "/lib" .. mod.name .. ".a -Wl,--no-whole-archive" }
            filter { }
        end
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
    local oldcwd = os.getcwd()
    os.chdir(mod._plugin._install_dir)

    if type(mod.type) ~= "string" or mod.type == "" then
        mod.type = "default"
    end

    local kindname = kind_by_module_type[mod.type]
    assert(kindname, "Unknown module type: '" .. mod.type .. "'.")

    local target_dir = target_dir_by_kind[kindname]
    assert(target_dir, "No target_dir known for kind: '" .. kindname .. "'.")

    project(mod.name)
        language "C++"
        kind(kindname)
        objdir(obj_dir)
        targetdir(target_dir)

        for key, value in orderedPairs(mod) do
            _try_handle_key(mod, key, value)
        end

        _platform_file_excludes()

    os.chdir(oldcwd)
end

function import_plugins(plugins)
    assert(type(plugins) == "table", "import_plugins expects a table")

    local imported = {}

    for _, plugin_path in ipairs(plugins) do
        local plugin_file = plugin_path

        if not path.hasextension(plugin_file, ".json") then
            plugin_file = path.join(plugin_file, "he_plugin.json")
        end

        plugin_file = path.join(path.getabsolute(_SCRIPT_DIR), plugin_file);

        local plugin = json.decode(io.readfile(plugin_file))

        if plugin.modules == nil then
            return
        end

        plugin._file_path = plugin_file
        install_plugin(plugin)

        for _, mod in ipairs(plugin.modules) do
            mod._plugin = plugin

            local existing = imported_modules[mod.name]
            assert(existing == nil, "Module '" .. mod.name .. "' was already provided by plugin '" .. (existing and existing._plugin.id or "") .. "', but plugin '" .. plugin.id .. "' also provides it.")

            -- TODO: Resolve "remove_*" keys in variants at this point.

            imported_modules[mod.name] = mod
            imported_modules_count = imported_modules_count + 1

            table.insert(imported, mod);
        end
    end

    for _, mod in ipairs(imported) do
        if mod.type ~= "test" then
            _module_project(mod)
        end
    end
end

function create_all_module_test_projects()
    for name, mod in orderedPairs(imported_modules) do
        if mod.type == "test" then
            _module_project(mod)
        end
    end
end

function add_module_keys(scope, keys)
    assert(type(keys) == "table", "add_module_keys expects a table of keys")

    local dst = nil
    if scope == "include" then
        dst = module_dependency_include_keys
    elseif scope == "link" then
        dst = module_dependency_link_keys
    else
        premake.error("Cannot add module keys to unknown scope: '" .. scope .. "'")
        return
    end

    for _, key in ipairs(keys) do
        local public_key = "public_" .. key
        local private_key = "private_" .. key

        table.insert(dst, { public_key, public_key })

        local handler = function (ctx, value)
            _G[key](value)
        end
        key_handlers[public_key] = handler
        key_handlers[private_key] = handler
    end
end
