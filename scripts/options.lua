-- Copyright Chad Engler

newoption {
    trigger = "slnfilename",
    default = "",
    description = "Set the generated solution name",
}

newoption {
    trigger = "static",
    description = "Statically link all modules, and disable hot reload functionality"
}

newoption {
    trigger = "windows_systemversion",
    default = "latest",
    description = "Set the windows SDK version to build against",
}
