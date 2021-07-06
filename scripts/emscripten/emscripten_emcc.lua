-- Copyright Chad Engler

local emscripten = premake.modules.emscripten

emscripten.emcc = {}

premake.tools.emcc = emscripten.emcc

local emcc = emscripten.emcc
local clang = premake.tools.clang
local config = premake.config

emcc.shared = {
    flags = {
        EmSIMD = "-msimd128",
        EmSSE = "-msse",
    },
}

--
-- Build a list of flags for the C preprocessor corresponding to the
-- settings in a particular project configuration.
--
-- @param cfg
--    The project configuration.
-- @return
--    An array of C preprocessor flags.
--

function emcc.getcppflags(cfg)
    local flags = clang.getcppflags(cfg)
    return flags
end

--
-- Build a list of C compiler flags corresponding to the settings in
-- a particular project configuration. These flags are exclusive
-- of the C++ compiler flags, there is no overlap.
--
-- @param cfg
--    The project configuration.
-- @return
--    An array of C compiler flags.
--

function emcc.getcflags(cfg)
    local clang_flags = clang.getcflags(cfg)
    local shared = config.mapFlags(cfg, emcc.shared)
    local flags = table.join(clang_flags, shared)

    for _, opt in ipairs(cfg.em_options) do
        table.insert(flags, "-s " .. opt)
    end

    return flags
end

--
-- Build a list of C++ compiler flags corresponding to the settings
-- in a particular project configuration. These flags are exclusive
-- of the C compiler flags, there is no overlap.
--
-- @param cfg
--    The project configuration.
-- @return
--    An array of C++ compiler flags.
--

function emcc.getcxxflags(cfg)
    local clang_flags = clang.getcxxflags(cfg)
    local shared = config.mapFlags(cfg, emcc.shared)
    local flags = table.join(clang_flags, shared)

    for _, opt in ipairs(cfg.em_options) do
        table.insert(flags, "-s " .. opt)
    end

    return flags
end

--
-- Returns a list of defined preprocessor symbols, decorated for
-- the compiler command line.
--
-- @param defines
--    An array of preprocessor symbols to define; as an array of
--    string values.
-- @return
--    An array of symbols with the appropriate flag decorations.
--

function emcc.getdefines(defines)
    local flags = clang.getdefines(defines)
    return flags
end

function emcc.getundefines(undefines)
    local flags = clang.getundefines(undefines)
    return flags
end

--
-- Returns a list of forced include files, decorated for the compiler
-- command line.
--
-- @param cfg
--    The project configuration.
-- @return
--    An array of force include files with the appropriate flags.
--

function emcc.getforceincludes(cfg)
    local flags = clang.getforceincludes(cfg)
    return flags
end

--
-- Returns a list of include file search directories, decorated for
-- the compiler command line.
--
-- @param cfg
--    The project configuration.
-- @param dirs
--    An array of include file search directories; as an array of
--    string values.
-- @return
--    An array of symbols with the appropriate flag decorations.
--

function emcc.getincludedirs(cfg, dirs)
    local flags = clang.getincludedirs(cfg, dirs)
    return flags
end

emcc.getrunpathdirs = clang.getrunpathdirs

--
-- Build a list of linker flags corresponding to the settings in
-- a particular project configuration.
--
-- @param cfg
--    The project configuration.
-- @return
--    An array of linker flags.
--

emcc.ldflags = {
    flags = {
        EmCpuProfiler = "--cpuprofiler",
        EmMemoryProfiler = "--memoryprofiler",
        EmThreadProfiler = "--threadprofiler",
        EmIgnoreDynamicLinking = "--ignore-dynamic-linking",
    },
    em_linkeroptimize = {
        Off = "--llvm-lto 0",
        Simple = "--llvm-lto 1",
        On = "--llvm-lto 2",
        Unsafe = "--llvm-lto 3",
    },
}

function emcc.getldflags(cfg)
    local flags = config.mapFlags(cfg, emcc.ldflags)

    local function addValueFlag(value, flag)
        if value then
            table.insert(flags, flag .. " \"" .. value .. "\"")
        end
    end

    addValueFlag(cfg.em_library, "--js-library")
    addValueFlag(cfg.em_prepend, "--pre-js")
    addValueFlag(cfg.em_append, "--post-js")
    addValueFlag(cfg.em_sourcemapbase, "--source-map-base")
    addValueFlag(cfg.em_embedfile, "--embed-file")
    addValueFlag(cfg.em_preloadfile, "--preload-file")
    addValueFlag(cfg.em_htmlshell, "--shell-file")

    for _, opt in ipairs(cfg.em_options) do
        table.insert(flags, "-s " .. opt)
    end

    if next(cfg.em_exportedfunctions) ~= nil then
        local export = "-s \"EXPORTED_FUNCTIONS=["
        for _, funcname in ipairs(cfg.em_exportedfunctions) do
            if _ > 0 then
                export = export .. ", "
            end
            export = export .. "'" .. funcname .. "'"
        end
        export = export .. "]\""
        table.insert(flags, export)
    end

    return flags
end

--
-- Build a list of additional library directories for a particular
-- project configuration, decorated for the tool command line.
--
-- @param cfg
--    The project configuration.
-- @return
--    An array of decorated additional library directories.
--

function emcc.getLibraryDirectories(cfg)
    local flags = clang.getLibraryDirectories(cfg)
    return flags
end

--
-- Build a list of libraries to be linked for a particular project
-- configuration, decorated for the linker command line.
--
-- @param cfg
--    The project configuration.
-- @param systemOnly
--    Boolean flag indicating whether to link only system libraries,
--    or system libraries and sibling projects as well.
-- @return
--    A list of libraries to link, decorated for the linker.
--

function emcc.getlinks(cfg, systemOnly)
    local flags = clang.getlinks(cfg, systemOnly)
    return flags
end

--
-- Return a list of makefile-specific configuration rules. This will
-- be going away when I get a chance to overhaul these adapters.
--
-- @param cfg
--    The project configuration.
-- @return
--    A list of additional makefile rules.
--

function emcc.getmakesettings(cfg)
    local flags = clang.getmakesettings(cfg)
    return flags
end

--
-- Retrieves the executable command name for a tool, based on the
-- provided configuration and the operating environment. I will
-- be moving these into global configuration blocks when I get
-- the chance.
--
-- @param cfg
--    The configuration to query.
-- @param tool
--    The tool to fetch, one of "cc" for the C compiler, "cxx" for
--    the C++ compiler, or "ar" for the static linker.
-- @return
--    The executable command name for a tool, or nil if the system"s
--    default value should be used.
--

emcc.tools = {
    cc = "emcc",
    cxx = "em++",
    ar = "emar",
}

function emcc.gettoolname(cfg, tool)
    if _ACTION == "gmake" or _ACTION == "gmake2" then
        if cfg.emccpath ~= nil then
            return path.join(cfg.emccpath, emcc.tools[tool])
        end
    end
    return emcc.tools[tool]
end
