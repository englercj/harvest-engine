-- Copyright Chad Engler

include "scripts/_setup.lua"

if he.project then
    if he.project.plugins then
        he.cwd_push(path.getdirectory(he.project_filename))
        he.import_plugins(he.project.plugins, he.project.plugin_import_options)
        he.cwd_pop()
    end

    if he.platforms then
        local host = os.host()
        local platform_names = he.get_platform_names(host)

        he.disable_all_platforms(host)
        for _, platform_name in ipairs(platform_names) do
            he.enable_platform(host, platform_name)
        end
    end
end

he.workspace()
    if he.project then
        if he.project.start_project then
            startproject(he.project.start_project)
        else
            startproject "he_editor"
        end
    end

    he.generate_module_projects()
