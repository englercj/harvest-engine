-- Copyright Chad Engler

local TOML = dofile("toml.lua")

-- Load the project file we're operating on
if not _OPTIONS.help then
    he.project_filename = _OPTIONS["he_project"]

    if not he.project_filename or #he.project_filename == 0 then
        he.project_filename = "he_project.toml"
    end

    local project_str = io.readfile(he.project_filename)

    if not project_str or #project_str == 0 then
        error("Project file does not exist, or is empty: " .. he.project_filename)
    end

    he.project = TOML.parse(project_str)

    if not he.project then
        error("Failed to parse project file: " .. he.project_filename)
    end

    printf("Loaded project '%s' from file: %s", he.project.name, he.project_filename)
end

-- Root directory of the project
he.root_dir = path.getdirectory(path.getabsolute(he.project_filename))

-- Name of the solution file (and workspace)
he.sln_name = _OPTIONS["he_slnfilename"]

if not he.sln_name or #he.sln_name == 0 then
    he.sln_name = he.slugify(he.project.name)
end

-- Detect the build dir
local to_dir = "build"

if _OPTIONS.to ~= nil and #_OPTIONS.to > 0 then
    to_dir = _OPTIONS.to
end

-- Build directories
he.build_dir            = path.join(he.root_dir, to_dir)
he.plugins_install_dir  = path.join(he.build_dir, "plugins")
he.projects_dir         = path.join(he.build_dir, "projects")
he.gen_dir              = path.join(he.build_dir, "generated")

-- Build directories based on target & configuration
he.target_build_dir     = path.join(he.build_dir, "%{cfg.platform:lower()}-%{cfg.buildcfg:lower()}")
he.target_bin_dir       = path.join(he.target_build_dir, "bin")
he.target_lib_dir       = path.join(he.target_build_dir, "lib/%{prj.name}")
he.target_obj_dir       = path.join(he.target_build_dir, "obj/%{prj.name}")
he.target_gen_dir       = path.join(he.target_build_dir, "generated")

-- Generated file paths
he.get_generated_dir = function (module_name)
    local plugin = he.get_module(module_name)._plugin
    local plugin_dir_name = he.slugify(plugin.id)
    return path.join(he.gen_dir, plugin_dir_name)
end

he.get_file_gen_dir = function (module_name)
    return "%{he.get_generated_dir('" .. module_name .. "')}/%{path.getrelative(he.get_module_install_dir('" .. module_name .. "', cfg.system), file.directory)}"
end

-- Module file paths
he.get_module_file_path = function (module_name)
    return he.get_module(module_name)._plugin._file_path
end

he.get_module_file_dir = function (module_name)
    return path.getdirectory(he.get_module_file_path(module_name))
end

-- Module install paths
he.get_module_install_dirs = function (module_name)
    local plugin = he.get_module(module_name)._plugin
    return plugin._install_dirs
end

he.get_module_install_dir = function (module_name, system)
    local install_dirs = he.get_module_install_dirs(module_name)
    return iif(install_dirs["*"] ~= nil, install_dirs["*"], install_dirs[system])
end
