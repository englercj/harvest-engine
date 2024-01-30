-- Copyright Chad Engler

-- ------------------------------------------------------------------------------------------------
-- Sorted table pair iteration

-- Equivalent of the pairs() function on tables, but iterates keys in alphabetical order.
function he.ordered_pairs(t)
    local keys = {}
    for key in pairs(t) do
        table.insert(keys, key)
    end
    table.sort(keys)

    local index = 0
    return function ()
        index = index + 1
        local key = keys[index]

        if key then
            return key, t[key]
        end
    end
end

-- ------------------------------------------------------------------------------------------------
-- Filter stack

local filter_stack = {}

-- Pushes a filter onto the stack
function he.filter_push(f)
    if f == nil then
        filter { }
    else
        filter(f)
    end
    table.insert(filter_stack, f)
end

-- Pushes a new filter onto the stack that is the combination of the provided filter values and
-- the currently active filter.
function he.filter_push_combine(f)
    local active_filter = he.filter_get_active()

    if active_filter == nil then
        he.filter_push(f)
    else
        local new_filter = table.join(active_filter, f)
        he.filter_push(new_filter)
    end
end

-- Pops the active filter off the stack and restores the state to the next filter
function he.filter_pop()
    table.remove(filter_stack)
    local prev_filter = he.filter_get_active()
    if prev_filter == nil then
        filter { }
    else
        filter(prev_filter)
    end
end

-- Gets the currently active filter on the stack (possibly nil)
function he.filter_get_active()
    return filter_stack[#filter_stack]
end

-- ------------------------------------------------------------------------------------------------
-- CWD Stack

local cwd_stack = {}

-- Pushes a directory onto the CWD stack
function he.cwd_push(dir)
    table.insert(cwd_stack, os.getcwd())
    os.chdir(dir)
end

-- Pops a directory off the CWD stack
function he.cwd_pop()
    local dir = table.remove(cwd_stack)
    if dir then
        os.chdir(dir)
    end
end

-- ------------------------------------------------------------------------------------------------
-- Table extensions

-- Returns true if this table looks like it is an array.
-- Because of lua there's no way to know for sure, but this should be right 99% of the time.
function table.is_array(t)
    local i = 0
    for _ in pairs(t) do
        i = i + 1
        if t[i] == nil then return false end
    end
    return true
end

-- Merges a series of tables into a new result, including concatinating arrays.
function table.deep_merge(...)
    local result = {}
    local args = {...}

    for _, arg in ipairs(args) do
        assert(type(arg) == "table", "deep_merge expects tables as arguments.")
        for key, value in pairs(arg) do
            local both_tables = type(result[key]) == "table" and type(value) == "table"
            local both_arrays = both_tables and table.is_array(result[key]) and table.is_array(value)

            if both_tables and both_arrays then
                local t = result[key]
                for _, el in ipairs(value) do
                    table.insert(t, el)
                end
            elseif both_tables then
                result[key] = table.deep_merge(result[key], value)
            else
                result[key] = value
            end
        end
    end

    return result
end

function table.length(t)
    local count = 0
    for _ in pairs(t) do
        count = count + 1
    end
    return count
end

-- ------------------------------------------------------------------------------------------------
-- Transform a string into a URL & filesystem safe slug

function he.slugify(str)
    return str:lower()
        :gsub("%s+", "-")       -- Replace spaces with -
        :gsub("[^%w-]+", "")    -- Remove all non-word chars
        :gsub("--+", "-")       -- Replace multiple - with single -
        :gsub("^-+", "")        -- Trim - from start of text
        :gsub("-+$", "")        -- Trim - from end of text
end

-- ------------------------------------------------------------------------------------------------
-- OS Extensions

-- Executes a command and returns the exit code and output
function he.execute(cmd)
    cmd = os.translateCommands(cmd)
    local file = io.popen(cmd, "r")
    local output = file:read("*all")
    local rc = { file:close() }

    -- rc[1] will be true, false or nil
    -- rc[3] will be the signal
    return rc[3], output
end

-- Same as he.execute, but formats the command string with the provided arguments
function he.executef(cmd, ...)
    cmd = string.format(cmd, ...)
    return he.execute(cmd)
end

-- Searches the system PATH variable to find the full path to the given program
function he.whereis(program)
    local is_windows = os.ishost("windows")
    local sep = iif(is_windows, ";", ":")
    local path_split = "[^" .. sep .. "]+"
    local env_path = os.getenv("PATH")
    if env_path then
        for dir in env_path:gmatch(path_split) do
            local fullpath = path.join(dir, program)
            if os.isfile(fullpath) then
                return fullpath
            end
            if is_windows and not program:match(".exe$") then
                local fullpath_exe = fullpath .. ".exe"
                if os.isfile(fullpath_exe) then
                    return fullpath_exe
                end
            end
        end
    end
    return nil
end
