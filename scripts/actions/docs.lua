-- Copyright Chad Engler

local p = premake

p.modules.docgen = {}
local m = p.modules.docgen

m._VERSION = p._VERSION

local bin_extension = iif(os.ishost("windows"), ".exe", "")
m.config = {
    output_directory = path.join(he.build_dir, "docs"),

    doxygen = {
        plugin_id = "doxygen.doxygen",
        exe_file = "doxygen" .. bin_extension,
        out_dir = "xml",

        doxyfile_path = "docs/config/Doxyfile",
        doxyfile_tmpname = os.tmpname(),
    },
    doxybook2 = {
        plugin_id = "matusnovak.doxybook2",
        exe_file = "bin/doxybook2" .. bin_extension,
        out_dir = "markdown",

        config_path = "docs/config/doxybook2.json",
        templates_dir = "docs/doxybook2_templates",
    },
    hugo = {
        plugin_id = "gohugoio.hugo",
        exe_file = "hugo" .. bin_extension,
        out_dir = "html",

        config_path = "docs/config/hugo.toml",
    },
}

local doxygen = m.config.doxygen
local doxybook2 = m.config.doxybook2
local hugo = m.config.hugo

local function get_exe_path(plugin_id, exe_file)
    local plugin = he.get_plugin(plugin_id)
    assert(plugin ~= nil, "No plugin found from plugin id: '%s'", plugin_id)
    return path.join(plugin._install_dirs[os.host()], exe_file)
end

local function ensure_out_dir(out_dir)
    local path = path.join(m.config.output_directory, out_dir)
    local result, err_msg = os.mkdir(path)
    if not result then
        p.error("Failed to create directory: %s", path)
        p.error("    %s", err_msg)
        return false
    end
    return true
end

local function run_doxygen(mod)
    -- Calculate our paths
    local cwd = os.getcwd()
    local plugin_dir = path.getdirectory(mod._plugin._file_path)
    local input_base_dir = path.getrelative(cwd, plugin_dir) .. "/"
    local doxygen_input = table.implode(mod.docs.inputs, input_base_dir, "", " ")

    -- Generate the doxyfile
    local doxyfile = doxygen.doxyfile
    doxyfile = doxyfile .. "\nINPUT = " .. doxygen_input
    doxyfile = doxyfile .. "\nXML_OUTPUT = " .. mod.name

    local result = io.writefile(doxygen.doxyfile_tmpname, doxyfile)
    if not result then
        p.error("Failed to write doxygen file to temp path: %s", doxygen.doxyfile_tmpname)
        return false
    end

    -- Execute doxygen for this module
    local doxygen_cmd = string.format("%s %s", doxygen.exe_path, doxygen.doxyfile_tmpname)
    verbosef("Executing doxygen: %s", doxygen_cmd)
    return os.execute(doxygen_cmd)
end

local function run_doxybook2(mod)
    local xml_path = path.join(m.config.output_directory, doxygen.out_dir, mod.name)
    local out_path = path.join(m.config.output_directory, doxybook2.out_dir, mod.name)
    local doxybook2_cmd = string.format("%s --input %s --output %s", doxybook2.exe_path, xml_path, out_path)

    if not ensure_out_dir(out_path) then
        return false
    end

    verbosef("Executing doxybook2: %s", doxybook2_cmd)
    return os.execute(doxybook2_cmd)
end

local function run_hugo(mod)
    local out_path = path.join(m.config.output_directory, hugo.out_dir, mod.name)
    local hugo_cmd = string.format("%s --config %s -d %s", hugo.exe_path, hugo.config_path, out_path)

    verbosef("Executing Hugo: %s", hugo_cmd)
    return os.execute(hugo_cmd)
end

local module_input_dirs = {}

newaction {
    trigger = "docs",
    description = "Generate HTML documentation files from source code.",

    onStart = function()
        print("Starting documentation generation")

        module_input_dirs = {}

        -- Create output directories
        ensure_out_dir(doxygen.out_dir)
        ensure_out_dir(doxybook2.out_dir)
        ensure_out_dir(hugo.out_dir)

        -- Calculate the exe path for each plugin
        doxygen.exe_path = get_exe_path(doxygen.plugin_id, doxygen.exe_file)
        doxybook2.exe_path = get_exe_path(doxybook2.plugin_id, doxybook2.exe_file)
        hugo.exe_path = get_exe_path(hugo.plugin_id, hugo.exe_file)

        -- Load and check the base doxyfile
        doxygen.doxyfile = io.readfile(doxygen.doxyfile_path)
        if not doxygen.doxyfile or doxygen.doxyfile == "" then
            p.error("Doxyfile is empty. Does the path '%s' exist?", doxygen.doxyfile_path)
            return
        end
        doxygen.doxyfile = doxygen.doxyfile .. "\nOUTPUT_DIRECTORY = " .. m.config.output_directory
        doxygen.doxyfile = doxygen.doxyfile .. "\nXML_OUTPUT = " .. doxygen.out_dir
    end,

    onProject = function(prj)
        local mod = he.get_module(prj.name)
        assert(mod ~= nil, "No module found from project named: '%s'", prj.name)

        -- Validate this module is configured to generate documentation
        if not mod.docs then
            verbosef("Skipping documentation generation for module '%s' because it does not include a 'docs' key", mod.name)
            return
        end
        assert(mod.docs.inputs and next(mod.docs.inputs) ~= nil, "Docs configuration for module '" .. mod.name .. "' is invalid. Inputs array is empty.")

        -- Calculate the input paths and add them to the list to be processed later in `execute`
        local cwd = os.getcwd()
        local plugin_dir = path.getdirectory(mod._plugin._file_path)
        local input_base_dir = path.getrelative(cwd, plugin_dir)

        for _, dir in ipairs(mod.docs.inputs) do
            local input = path.join(input_base_dir, dir)
            table.insert(module_input_dirs, input)
        end
    end,

    execute = function()
        -- Generate the doxyfile
        local doxygen_input = table.implode(module_input_dirs, "", "", " ")
        local doxyfile = doxygen.doxyfile
        doxyfile = doxyfile .. "\nINPUT = " .. doxygen_input

        local result = io.writefile(doxygen.doxyfile_tmpname, doxyfile)
        if not result then
            p.error("Failed to write doxygen file to temp path: %s", doxygen.doxyfile_tmpname)
            return
        end

        -- Execute doxygen to generate the XML data
        local doxygen_cmd = string.format("%s %s", doxygen.exe_path, doxygen.doxyfile_tmpname)
        verbosef("Executing doxygen: %s", doxygen_cmd)
        if not os.execute(doxygen_cmd) then
            p.error("Failed to execute doxygen using command: %s", doxygen_cmd)
            return
        end

        -- Execute doxybook2 to convert the XML data into markdown
        local xml_path = path.join(m.config.output_directory, doxygen.out_dir)
        local md_path = path.join(m.config.output_directory, doxybook2.out_dir)
        local doxybook2_cmd = string.format("%s --input %s --output %s --config %s --templates %s",
            doxybook2.exe_path, xml_path, md_path, doxybook2.config_path, doxybook2.templates_dir)
        verbosef("Executing doxybook2: %s", doxybook2_cmd)
        if not os.execute(doxybook2_cmd) then
            p.error("Failed to execute doxybook2 using command: %s", doxybook2_cmd)
            return
        end

        -- Execute hugo to convert the markdown into HTML
        local html_path = path.join(m.config.output_directory, hugo.out_dir)
        local hugo_cmd = string.format("%s --config %s -d %s", hugo.exe_path, hugo.config_path, html_path)
        verbosef("Executing Hugo: %s", hugo_cmd)
        if not os.execute(hugo_cmd) then
            p.error("Failed to execute Hugo using command: %s", hugo_cmd)
            return
        end
    end,

    onEnd = function()
        print("Documentation generation complete")
        os.remove(doxygen.doxyfile_tmpname)
    end,
}

he.add_module_key {
    key = "docs",
    scope = "private",
    type = "table",
    desc = "an object for configuring documentation generation",
    handler = function (ctx, values) end,
}
