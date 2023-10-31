-- Copyright Chad Engler

include "scripts/_setup.lua"

he.workspace()
    if he.project and he.project.start_project then
        startproject(he.project.start_project)
    else
        startproject "he_editor"
    end

    if he.project and he.project.plugins then
        he.cwd_push(path.getdirectory(he.project_filename))
        he.import_plugins(he.project.plugins, he.project.plugin_import_options)
        he.cwd_pop()
    end

    he.generate_module_projects()
