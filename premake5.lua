-- Copyright Chad Engler

include "scripts/_setup.lua"

-- Import the plugins that are referenced by the project, if any
if he.project and he.project.plugins then
    he.cwd_push(path.getdirectory(he.project_filename))
    he.import_plugins(he.project.plugins, he.project.plugin_import_options)
    he.cwd_pop()
end

-- Enable only the platforms listed by the project, if any
if he.project and he.project.platforms then
    local host = os.host()
    for _, platform_name in ipairs(he.project.platforms) do
        he.enable_platform(host, platform_name)
    end
else
    he.enable_all_platforms(os.host())
end

-- Choose the default startup project
local start_project = "he_editor"
if he.project and he.project.start_project then
    start_project = he.project.start_project
end

-- Generate the workspace and project
he.generate_workspace {
    start_project = start_project,
}
he.generate_module_projects()
