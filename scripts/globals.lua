-- Copyright Chad Engler

root_dir = path.getabsolute(path.join(_SCRIPT_DIR, ".."))

sln_name = _OPTIONS["slnfilename"]

if #sln_name == 0 then
    sln_name = path.getbasename(root_dir) .. "_" .. os.target()
end

build_dir           = path.join(root_dir, "build")
plugin_install_dir  = path.join(build_dir, "plugins")
projects_dir        = path.join(build_dir, "projects/%{os.target()}")

target_build_dir    = path.join(build_dir, "%{os.target()}-%{cfg.architecture}-%{cfg.buildcfg:lower()}")
target_bin_dir      = path.join(target_build_dir, "bin")
target_lib_dir      = path.join(target_build_dir, "lib/%{prj.name}")
target_obj_dir      = path.join(target_build_dir, "obj/%{prj.name}")
target_gen_dir      = path.join(target_build_dir, "generated")

target_file_gen_dir = "%{get_generated_dir(prj.name)}/%{path.getrelative(get_module(prj.name)._plugin._install_dir, path.getdirectory(file.abspath))}"

function get_generated_dir(project_name)
    return path.join(target_gen_dir, project_name)
end
