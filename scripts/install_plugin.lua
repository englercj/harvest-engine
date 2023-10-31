-- Copyright Chad Engler

local last_download_progress = 0

local function _hex_to_char(x)
    return string.char(tonumber(x, 16))
end

local function _unescape_url(url)
    return url:gsub("%%(%x%x)", _hex_to_char)
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
    verbosef("Downloading archive to path %s", fpath)
    last_download_progress = 0
    local result_str, response_code = http.download(url, fpath, {
        sslverifypeer = 0,
        progress = _download_progress,
    })

    print(result_str, response_code)
    return result_str, response_code
end

local function _download_archive(name, url, dir, archive_name)
    os.mkdir(dir)

    printf("Downloading archive for %s (%s)", name, url)

    local fpath = path.join(dir, archive_name)

    local result_str, response_code = _download_file(url, fpath)
    return result_str == "OK"
end

local function _install_from_archive(name, url, archive_name, extract_dir_name)
    local digest = string.sha1(name .. url)
    local archive_dir = path.join(he.plugin_install_dir, name)

    local vfile = path.join(archive_dir, ".plugin_digest")
    local installed_version = io.readfile(vfile);

    local extract_dir = ""
    if extract_dir_name == nil then
        extract_dir = archive_dir
    else
        extract_dir = path.join(archive_dir, extract_dir_name)
    end

    extract_dir = _unescape_url(extract_dir)

    if installed_version == digest then
        verbosef("Plugin '%s' matches local digest, skipping.", name)
        return extract_dir
    end

    verbosef("Installing plugin '%s' from archive '%s'", name, url)

    if _download_archive(name, url, archive_dir, archive_name) == false then
        premake.error("Failed to install archive for " .. name .. " from " .. url)
        return
    end

    printf("Extracting %s...", archive_name)

    os.mkdir(extract_dir)

    local fpath = path.join(archive_dir, archive_name)

    if os.host() == "windows" then
        -- zip.extract() chokes on large files so invoke a system utility instead
        os.executef("%%WINDIR%%/system32/tar.exe -xf \"%s\" -C \"%s\"", fpath, extract_dir)
    elseif archive_name:find(".zip") ~= nil then
        -- zip.extract() chokes on large files so invoke a system utility instead
        os.executef("unzip \"%s\" -d \"%s\"", fpath, extract_dir)
    elseif archive_name:find(".tar") ~= nil then
        os.executef("tar xf \"%s\" -C \"%s\"", fpath, extract_dir)
    else
        premake.error("Failed to extract %s, unrecognized extension.", archive_name)
        return false
    end

    io.writefile(vfile, digest)
    return extract_dir
end

local function _install_from_github(name, source)
    local org
    local repo
    local version = "master"

    verbosef("Installing plugin '%s' from github '%s'", name, source)

    -- Tokenize the string
    local t = 0
    for s in string.gmatch(source, "[^/#]+") do
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
    local archive_name = org .. "_" .. repo .. "_" .. version .. ".zip"
    local install_dir = _install_from_archive(name, url, archive_name)

    return path.join(install_dir, repo .. "-" .. trimmed_version)
end

local function _install_from_bitbucket(name, source)
    local org
    local repo
    local version = "master"

    verbosef("Installing plugin '%s' from bitbucket '%s'", name, source)

    -- Tokenize the string
    local t = 0
    for s in string.gmatch(source, "[^/#]+") do
        if t == 0 then org = s
        elseif t == 1 then repo = s
        elseif t == 2 then version = s
        end
        t = t + 1
    end

    local url = "https://bitbucket.org/" .. org .. "/" .. repo .. "/get/" .. version .. ".zip"
    local archive_name = org .. "_" .. repo .. "_" .. version .. ".zip"
    local install_dir = _install_from_archive(name, url, archive_name)

    local dirs = os.matchdirs(path.join(install_dir, org .. "-" .. repo .. "-*"))
    assert(type(dirs[1]) == "string", "Failed to find install directory for bitbucket install of: " .. name)

    return dirs[1]
end

local function _install_from_nuget(name, source)
    -- https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.210818001
    local package_name
    local version

    verbosef("Installing plugin '%s' from nuget '%s'", name, source)

    -- Tokenize the string
    local t = 0
    for s in string.gmatch(source, "[^#]+") do
        if t == 0 then package_name = s
        elseif t == 1 then version = s
        end
        t = t + 1
    end

    local url = "https://www.nuget.org/api/v2/package/" .. package_name .. "/" .. version
    local archive_name = package_name .. "_" .. version .. ".zip"
    local extract_dir_name = package_name .. "-" .. version
    local install_dir = _install_from_archive(name, url, archive_name, extract_dir_name)

    return install_dir
end

return function (plugin)
    local i = plugin.install
    local install_dir = path.getdirectory(plugin._file_path)

    -- If the plugin doesn't specify an install block then use the he_plugin file's path as
    -- the install path and return that it is enabled.
    if i == nil then
        verbosef("Plugin '%s' has no install block, skipping installation.", plugin.id)
        return true, install_dir
    end

    -- Check if the target system is valid for this plugin to be imported on, by default plugins
    -- are assumed to work on all systems
    local valid_targets = iif(i.valid_targets, i.valid_targets, { os.target() })
    local is_valid_target = false
    for _, v in ipairs(valid_targets) do
        if os.istarget(v) then
            is_valid_target = true
            break
        end
    end

    if not is_valid_target then
        verbosef("Skipping install of plugin '%s', current target is not listed in its 'valid_targets' key.", plugin.id)
        return false, install_dir
    end

    local valid_hosts = iif(i.valid_hosts, i.valid_hosts, { os.host() })
    local is_valid_host = false
    for _, v in ipairs(valid_hosts) do
        if os.ishost(v) then
            is_valid_host = true
            break
        end
    end

    if not is_valid_host then
        verbosef("Skipping install of plugin '%s', current host is not listed in its 'valid_hosts' key.", plugin.id)
        return false, install_dir
    end

    -- Check for install sources and perform the install
    if i.github ~= nil then
        install_dir = _install_from_github(plugin.id, i.github)
    elseif i.bitbucket ~= nil then
        install_dir = _install_from_bitbucket(plugin.id, i.bitbucket)
    elseif i.nuget ~= nil then
        install_dir = _install_from_nuget(plugin.id, i.nuget)
    elseif i.archive ~= nil then
        local target = iif(i.index_archive_by_host, os.host(), os.target())
        local url = i.archive
        if type(url) == "table" then
            url = url[target]
        end
        assert(type(url) == "string" and url ~= "", "Bad source when installing archive of '" .. plugin.id .. "' for '" .. target .. "'.")

        local extract_dir = i.base_path == nil and path.getbasename(url) or ""
        install_dir = _install_from_archive(plugin.id, url, path.getname(url), extract_dir)
    elseif i.source ~= nil then
        install_dir = path.join(install_dir, i.source)
    else
        verbosef("Plugin '%s' has no install source, nothing will be downloaded.", plugin.id)
    end

    if i.base_path ~= nil then
        install_dir = path.join(install_dir, i.base_path)
    end

    -- Run the install scripts if specified
    if i.exec ~= nil then
        local func = include(i.exec)
        func(plugin)
    end

    return true, install_dir
end
