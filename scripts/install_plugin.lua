-- Copyright Chad Engler

local targetSysId = (os.istarget("windows") and "windows") or (os.istarget("linux") and "linux")
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

local function _download_archive(name, source, dir)
    os.mkdir(dir)

    local url = source

    if type(source) == "table" then
        url = source[targetSysId]
    end

    assert(type(url) == "string" and url ~= "", "Bad url when installing archive of " .. name .. " for " .. targetSysId)

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

local function _install_from_github(name, source)
    local org
    local repo
    local version = "master"

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
    local install_dir = _install_from_archive(name, url)

    return path.join(install_dir, repo .. "-" .. trimmed_version)
end

local function _install_from_bitbucket(name, source)
    local org
    local repo
    local version = "master"

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
    local install_dir = _install_from_archive(name, url)

    local dirs = os.matchdirs(path.join(install_dir, org .. "-" .. repo .. "-*"))
    assert(type(dirs[1]) == "string", "Failed to find install directory for bitbucket install of: " .. name)

    return dirs[1]
end

return function (plugin)
    if type(plugin.source_github) == "string" then
        plugin._install_dir = _install_from_github(plugin.id, plugin.source_github)
    elseif type(plugin.source_bitbucket) == "string" then
        plugin._install_dir = _install_from_bitbucket(plugin.id, plugin.source_bitbucket)
    elseif type(plugin.source_archive) == "string" then
        plugin._install_dir = _install_from_archive(plugin.id, plugin.source_archive)
    elseif type(plugin.source) == "string" then
        plugin._install_dir = path.join(path.getdirectory(plugin._file_path), plugin.source)
    else
        plugin._install_dir = path.getdirectory(plugin._file_path)
    end

    if type(plugin.source_basepath) == "string" then
        plugin._install_dir = path.join(plugin._install_dir, plugin.source_basepath)
    end
end
