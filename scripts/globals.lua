-- Copyright Chad Engler

-- Root directory of the project
he.root_dir = path.getabsolute(_MAIN_SCRIPT_DIR)

-- Name of the solution file (and workspace)
he.sln_name = _OPTIONS.slnfilename

if #he.sln_name == 0 then
    he.sln_name = path.getbasename(he.root_dir) .. "_" .. os.target()
end

-- Detect the build dir
local to_dir = "build"

if _OPTIONS.to ~= nil and #_OPTIONS.to > 0 then
    to_dir = _OPTIONS.to
end

-- Build directories
he.build_dir            = path.join(he.root_dir, to_dir)
he.plugin_install_dir   = path.join(he.build_dir, "plugins")
he.projects_dir         = path.join(he.build_dir, "projects/%{os.target()}")
he.gen_dir              = path.join(he.build_dir, "generated")

-- Build directories based on target & configuration
he.target_build_dir     = path.join(he.build_dir, "%{os.target()}-%{cfg.architecture}-%{cfg.buildcfg:lower()}")
he.target_bin_dir       = path.join(he.target_build_dir, "bin")
he.target_lib_dir       = path.join(he.target_build_dir, "lib/%{prj.name}")
he.target_obj_dir       = path.join(he.target_build_dir, "obj/%{prj.name}")
he.target_gen_dir       = path.join(he.target_build_dir, "generated")

-- Generated file paths
he.file_gen_dir = "%{he.get_generated_dir(prj.name)}/%{path.getrelative(he.get_module(prj.name)._plugin._install_dir, file.directory)}"
he.get_generated_dir = function (project_name)
    local plugin = he.get_module(project_name)._plugin
    local plugin_dir_name = plugin.id:gsub("%.", "_")
    return path.join(he.gen_dir, plugin_dir_name)
end

-- Generated file paths based on target & configuration
he.target_file_gen_dir = "%{he.get_target_generated_dir(prj.name)}/%{path.getrelative(he.get_module(prj.name)._plugin._install_dir, file.directory)}"
he.get_target_generated_dir = function (project_name)
    local plugin = he.get_module(project_name)._plugin
    local plugin_dir_name = plugin.id:gsub("%.", "_")
    return path.join(he.target_gen_dir, plugin_dir_name)
end
