-- Copyright Chad Engler

newoption {
    trigger = "slnfilename",
    default = "",
    description = "Set the generated solution name",
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
