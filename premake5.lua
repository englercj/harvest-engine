-- Copyright Chad Engler

include "scripts/_setup.lua"

he.workspace()
    if he.project and he.project.startproject then
        startproject(he.project.startproject)
    else
        startproject "he_editor"
    end

    if he.project and he.project.import_plugins then
        he.cwd_push(path.getdirectory(he.project_filename))
        he.import_plugins(he.project.import_plugins)
        he.cwd_pop()
    end

    he.generate_module_projects()
