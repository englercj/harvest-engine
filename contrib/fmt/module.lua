-- Copyright Chad Engler

return {
    name = "fmt",
    github = "fmtlib/fmt#7.1.3",

    when_included = function ()
        includedirs { "include" }
    end,

    when_linked = function ()
        links { "fmt" }
    end,

    module_project = function ()
        include_modules { "fmt" }

        files {
            path.join("include/**"),
            path.join("src/**"),
        }
    end,
}
