-- Copyright Chad Engler

local p = premake
local api = p.api

he.add_module_key {
    key = "docs",
    scope = "private",
    type = "table",
    desc = "an object for configuring documentation generation",
    handler = function (ctx, values) end,
}
