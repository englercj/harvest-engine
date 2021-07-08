-- Copyright Chad Engler

return {
    name = "he_test_runner",
    kind = "ConsoleApp",

    module_project = function ()
        files { "main.cpp" }

        use_modules { "he_core" }

        use_all_module_tests()
    end,
}
