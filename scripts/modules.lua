-- Copyright Chad Engler

local sysId = (os.ishost("windows") and "windows") or (os.ishost("linux") and "linux")
local last_download_progress = 0

local target_dir_table = {
    ConsoleApp = bin_dir,
    WindowedApp = bin_dir,
    SharedLib = bin_dir,
    StaticLib = lib_dir,
}

local imported_modules = {}

local valid_include_keys = {
    "defines",
    "flags",
    "includedirs",
    "include_modules",
    "resincludedirs",
    "sysincludedirs",
}

local valid_link_keys = {
    "libdirs",
    "link_modules",
    "links",
    "linkoptions",
}

local valid_use_keys = {}
for i,v in ipairs(valid_include_keys) do table.insert(valid_use_keys, v) end
for i,v in ipairs(valid_link_keys) do table.insert(valid_use_keys, v) end

local path_value_keys = {
    includedirs = true,
    libdirs = true,
}

local function _run_module_func(mod, func_name)
    local oldcwd = os.getcwd()
    os.chdir(mod.install_dir)
    mod[func_name](mod)
    filter { }
    os.chdir(oldcwd)
end

local function _download_progress(total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);

    if percent > last_download_progress and percent % 2 == 0 then
        local diff = percent - last_download_progress;
        last_download_progress = percent
        io.write(string.rep(".", diff / 2));
    end
end

local function _download_file(url, fpath)
    last_download_progress = 0
    local result_str, response_code = http.download(url, fpath, {
        sslverifypeer = 0,
        progress = _download_progress,
    })

    print(result_str, response_code)
    return result_str, response_code
end

local function _download_archive(name, source, dir)
    os.mkdir(dir)

    local url = source

    if type(source) == "table" then
        url = source[sysId]
    end

    assert(type(url) == "string" and url ~= "", "Bad url when installing archive of " .. name .. " for " .. sysId)

    printf("Downloading archive: %s (%s)", name, url)

    local fname = url:match("([^/]+)$");
    local fpath = path.join(dir, fname)

    local result_str, response_code = _download_file(url, fpath)

    if result_str == "OK" then
        printf("Extracting %s...", fname)

        if fname:find(".zip") ~= nil then
            zip.extract(fpath, dir)
        elseif fname:find(".tar") ~= nil then
            -- TODO: This doesn"t work on windows (MINGW)
            os.executef("tar xf "%s" -C "%s"", fpath, dir)
        else
            printf("FAILED to extract %s, unrecognized extension.", fname)
            return false
        end

        return true
    end

    return false
end

local function _install_from_archive(name, source)
    local dir = path.join(dep_dir, name)
    local digest = string.sha1(name .. source)

    local vfile = path.join(dir, ".dependency_digest")
    local installed_version = io.readfile(vfile);

    if installed_version ~= digest then
        if _download_archive(name, source, dir) == false then
            premake.error("FAILED to install archive " .. name)
            return
        end

        io.writefile(vfile, digest)
    end

    return dir
end

local function _install_from_github(mod)
    local org
    local repo
    local version = "master"

    -- Tokenize the string
    local t = 0
    for s in string.gmatch(mod.github, "[^/#]+") do
        if t == 0 then org = s
        elseif t == 1 then repo = s
        elseif t == 2 then version = s
        end
        t = t + 1
    end

    local trimmed_version = version

    if version:sub(1, 1) == "v" then
        trimmed_version = version:sub(2)
    end

    local url = "https://github.com/" .. org .. "/" .. repo .. "/archive/" .. version .. ".zip"
    local install_dir = _install_from_archive(mod.name, url)

    mod.install_dir = path.join(install_dir, repo .. "-" .. trimmed_version)
end

local function _install_from_bitbucket(mod)
    local org
    local repo
    local version = "master"

    -- Tokenize the string
    local t = 0
    for s in string.gmatch(mod.bitbucket, "[^/#]+") do
        if t == 0 then org = s
        elseif t == 1 then repo = s
        elseif t == 2 then version = s
        end
        t = t + 1
    end

    local url = "https://bitbucket.org/" .. org .. "/" .. repo .. "/get/" .. version .. ".zip"
    local install_dir = _install_from_archive(mod.name, url)

    local dirs = os.matchdirs(path.join(install_dir, org .. "-" .. repo .. "-*"))
    assert(type(dirs[1]) == "string", "Failed to find install directory for bitbucket install of: " .. name)

    mod.install_dir = dirs[1]
end

local function _install_module(mod)
    if type(mod.github) == "string" then
        _install_from_github(mod)
    elseif type(mod.bitbucket) == "string" then
        _install_from_bitbucket(mod)
    elseif type(mod.archive) == "string" then
        mod.install_dir = _install_from_archive(mod.name, mod.archive)
        if type(mod.basepath) == "string" then
            mod.install_dir = path.join(mod.install_dir, mod.basepath)
        end
    elseif type(mod.source) == "string" then
        mod.install_dir = path.join(mod._mod_dir, mod.source)
    elseif type(mod.install) == "function" then
        mod.install(mod)
    end
end

function import_modules(modules)
    assert(type(modules) == "table", "import_modules expects a table")

    local imported = {}

    for _, mod_path in ipairs(modules) do
        local mod_file = mod_path

        if not path.hasextension(mod_file, ".lua") then
            mod_file = path.join(mod_file, "module.lua")
        end

        mod_file = path.join(path.getabsolute(_SCRIPT_DIR), mod_file);

        local mod = dofile(mod_file)

        assert(imported_modules[mod.name] == nil, "Module already imported: " .. mod.name)

        mod._mod_file = mod_file
        mod._mod_dir = path.getdirectory(mod_file)
        mod.install_dir = mod._mod_dir

        _install_module(mod)

        imported_modules[mod.name] = mod

        table.insert(imported, mod);
    end

    for _, mod in ipairs(imported) do
        module(mod.name, mod.kind)
        _run_module_func(mod, "module_project")
    end
end

function import_all_module_tests()
    for name, mod in pairs(imported_modules) do
        if mod.test_project ~= nil then
            mod.test_project_name = mod.name .. "__tests"
            module(mod.test_project_name, mod.kind)
            _run_module_func(mod, "test_project")
        end
    end
end

function use_all_module_tests()
    for name, mod in orderedPairs(imported_modules) do
        if mod.test_project_name ~= nil then
            use_modules { mod.name }
            links { mod.test_project_name }

            filter { "toolset:msc-*", "language:C++" }
                linkoptions { "/WHOLEARCHIVE:" .. mod.test_project_name }
            filter { "toolset:gcc or clang" }
                linkoptions { "-Wl,--whole-archive %{lib_base_dir}/" .. mod.test_project_name .. "/lib" .. mod.test_project_name .. ".a -Wl,--no-whole-archive" }
            filter {}
        end
    end
end

function add_module_keys(scope, keys)
    assert(type(keys) == "table", "add_module_keys expects a table of keys")

    local dst = nil
    if scope == "include" then
        dst = valid_include_keys
    elseif scope == "link" then
        dst = valid_link_keys
    else
        premake.error("Cannot add module key ('" .. key .. "') to unknown scope: '" .. scope .. "'")
        return
    end

    for _, key in ipairs(keys) do
        table.insert(dst, key)
        table.insert(valid_use_keys, key)
    end
end

function get_all_modules()
    return imported_modules
end

function get_module(name)
    if name == nil then name = prj.name end

    local mod = imported_modules[name]

    assert(mod ~= nil, "Module '" .. name .. "' was referenced but not imported.")
    return mod
end

function use_modules(dep_table)
    assert(type(dep_table) == "table", "use_module expects a table")
    include_modules(dep_table)
    link_modules(dep_table)
end

function include_modules(dep_table)
    assert(type(dep_table) == "table", "include_module expects a table")

    for _, name in ipairs(dep_table) do
        local mod = get_module(name)
        if mod.when_linked ~= nil then
            _run_module_func(mod, "when_included")
        end
    end
end

function link_modules(dep_table)
    assert(type(dep_table) == "table", "link_module expects a table")

    for _, name in ipairs(dep_table) do
        local mod = get_module(name)
        if mod.when_linked ~= nil then
            _run_module_func(mod, "when_linked")
        end
    end
end

function module(name, kindname)
    if kindname == nil then kindname = "StaticLib" end

    project(name)
    kind(kindname)
    objdir(obj_dir)

    language "C++"

    local target_dir = target_dir_table[kindname]

    if target_dir ~= nil then
        targetdir(target_dir)
    end
end

function platform_excludes()
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
