-- Copyright Chad Engler

newoption {
    trigger = "slnfilename",
    default = "",
    description = "Set the generated solution name",
}

-- TODO: Get rid of this, and the "default" switching type concept completely.
newoption {
    trigger = "static",
    description = "Statically link all modules, and disable hot reload functionality"
}

newoption {
    trigger = "windows_systemversion",
    default = "latest",
    description = "Set the windows SDK version to build against",
}

newoption {
    trigger = "asan",
    description = "Enable AddressSanitizer, disables incremental linking and edit-and-continue."
}
