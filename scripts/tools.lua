-- Copyright Chad Engler

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
            target_bin_dir .. "/bin2c " .. opt .. "-n c_%{file.name:gsub('[%.-]', '_')} -f %{file.abspath} -o " .. target_file_gen_dir .. "/%{file.name}.h",
        }
        buildoutputs {
            target_file_gen_dir .. "/%{file.name}.h",
        }

    filter { }
end
