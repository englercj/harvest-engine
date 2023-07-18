-- Copyright Chad Engler

newoption {
    trigger = "he_project",
    default = "",
    description = "Path to the project file",
}

newoption {
    trigger = "he_slnfilename",
    default = "",
    description = "Set the generated solution name. Defaults to the project directory name + the target platform.",
}

newoption {
    trigger = "he_windows_systemversion",
    default = "latest",
    description = "Set the windows SDK version to build against. Defaults to 'latest'.",
}

newoption {
    trigger = "asan",
    description = "Enable AddressSanitizer, disables incremental linking and edit-and-continue."
}
