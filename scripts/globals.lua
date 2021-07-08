-- Copyright Chad Engler

root_dir = path.getabsolute(path.join(_SCRIPT_DIR, ".."))

newoption {
    trigger     = "slnfilename",
    description = "Set the generated solution name",
    default     = "",
}

newoption { trigger = "windows_systemversion", default = "latest", description = "systemversion" }

sln_name = _OPTIONS["slnfilename"]

if #sln_name == 0 then
    sln_name = path.getbasename(root_dir)
end

build_dir   = path.join(root_dir, "build")
bin_dir     = path.join(build_dir, "bin/%{os.target()}-%{cfg.shortname}")
temp_dir    = path.join(build_dir, "temp")
dep_dir     = path.join(build_dir, "dependencies")
lib_base_dir= path.join(build_dir, "lib/%{os.target()}-%{cfg.shortname}")
lib_dir     = path.join(lib_base_dir, "%{lib_base_dir}/%{prj.name}")
obj_dir     = path.join(build_dir, "obj/%{os.target()}-%{cfg.shortname}/%{prj.name}")
project_dir = path.join(build_dir, "projects")
gen_dir     = path.join(build_dir, "generated")

-- Helpers for ordered key iteration
