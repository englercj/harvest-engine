-- Copyright Chad Engler

local p = premake
local last_download_progress = 0

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

local function _download_archive(name, source, dir, archiveName)
    os.mkdir(dir)

    local target = os.target()
    local url = source

    if type(source) == "table" then
        url = source[target]
    end

    assert(type(url) == "string" and url ~= "", "Bad url when installing archive of '" .. name .. "' for '" .. target .. "'.")

    printf("Downloading archive: %s (%s)", name, url)

    local fpath = path.join(dir, archiveName)

    local result_str, response_code = _download_file(url, fpath)
    return result_str == "OK"
end

local function _install_from_archive(name, source, archiveName, extractDirName)
    local digest = string.sha1(name .. source)
    local archiveDir = path.join(plugin_install_dir, name)

    local vfile = path.join(archiveDir, ".plugin_digest")
    local installed_version = io.readfile(vfile);

    if installed_version ~= digest then
        if _download_archive(name, source, archiveDir, archiveName) == false then
            p.error("Failed to install archive " .. name)
            return
        end

        printf("Extracting %s...", archiveName)

        local extractDir = ""
        if extractDirName == nil then
            extractDir = archiveDir
        else
            extractDir = path.join(archiveDir, extractDirName)
        end

        os.mkdir(extractDir)

        local fpath = path.join(archiveDir, archiveName)

        if archiveName:find(".zip") ~= nil then
            zip.extract(fpath, extractDir)
        elseif archiveName:find(".tar") ~= nil then
            os.executef("tar xf \"%s\" -C \"%s\"", fpath, extractDir)
        else
            p.error("Failed to extract %s, unrecognized extension.", archiveName)
            return false
        end

        io.writefile(vfile, digest)
    end

    return archiveDir
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
    local archiveName = org .. "_" .. repo .. "_" .. version .. ".zip"
    local install_dir = _install_from_archive(name, url, archiveName)

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
    local archiveName = org .. "_" .. repo .. "_" .. version .. ".zip"
    local install_dir = _install_from_archive(name, url, archiveName)

    local dirs = os.matchdirs(path.join(install_dir, org .. "-" .. repo .. "-*"))
    assert(type(dirs[1]) == "string", "Failed to find install directory for bitbucket install of: " .. name)

    return dirs[1]
end

local function _install_from_nuget(name, source)
    -- https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.210818001
    local package_name
    local version

    -- Tokenize the string
    local t = 0
    for s in string.gmatch(source, "[^#]+") do
        if t == 0 then package_name = s
        elseif t == 1 then version = s
        end
        t = t + 1
    end

    local url = "https://www.nuget.org/api/v2/package/" .. package_name .. "/" .. version
    local archiveName = package_name .. "_" .. version .. ".zip"
    local extractDirName = package_name .. "-" .. version
    local install_dir = _install_from_archive(name, url, archiveName, extractDirName)

    return path.join(install_dir, extractDirName)
end

return function (plugin)
    local i = plugin.install

    if i == nil then
        plugin._install_dir = path.getdirectory(plugin._file_path)
    elseif i.github ~= nil then
        plugin._install_dir = _install_from_github(plugin.id, i.github)
    elseif i.bitbucket ~= nil then
        plugin._install_dir = _install_from_bitbucket(plugin.id, i.bitbucket)
    elseif i.nuget ~= nil then
        plugin._install_dir = _install_from_nuget(plugin.id, i.nuget)
    elseif i.archive ~= nil then
        plugin._install_dir = _install_from_archive(plugin.id, i.archive)
    elseif i.source ~= nil then
        plugin._install_dir = path.join(path.getdirectory(plugin._file_path), i.source)
    else
        p.error("Plugin '" .. plugin.id .. "' has an install key, but no recognized source is specified.")
        return
    end

    if i ~= nil and i.basepath ~= nil then
        plugin._install_dir = path.join(plugin._install_dir, i.basepath)
    end
end
