-- Copyright Chad Engler

function get_generated_dir(project_name)
    return path.join(gen_dir, project_name)
end

local _gen_out_dir = "%{get_generated_dir(prj.name)}/%{path.getrelative(getpackage(prj.name)._pkg_dir, path.getdirectory(file.abspath))}"

function bin2c_compile(glob, options)
    local opt = ""

    if not options then
        options = {}
    end

    if options.text then
        opt = opt .. "-t "
    end
    if options.compress then
        opt = opt .. "-c "
    end

    files { glob }
    dependson { "bin2c" }

    filter { "files:" .. glob }
        compilebuildoutputs "on"
        buildmessage "Encoding file %{file.abspath}"
        buildcommands {
            bin_dir .. "/bin2c " .. opt .. "-n c_%{file.name:gsub('[%.-]', '_')} -f %{file.abspath} -o " .. _gen_out_dir .. "/%{file.name}.h",
        }
        buildoutputs {
            _gen_out_dir .. "/%{file.name}.h",
        }

    filter { }
end
