-- Copyright Chad Engler

-- ------------------------------------------------------------------------------------------------
-- Sorted table pair iteration

local function _genOrderedIndex(t)
    local orderedIndex = {}
    for key in pairs(t) do
        table.insert(orderedIndex, key)
    end
    table.sort(orderedIndex)
    return orderedIndex
end

local function _orderedNext(t, state)
    -- Equivalent of the next function, but returns the keys in the alphabetic order.
    -- We use a temporary ordered key table that is stored in the table being iterated.

    local key = nil
    if state == nil then
        -- the first time, generate the index
        t.__orderedIndex = _genOrderedIndex(t)
        key = t.__orderedIndex[1]
    else
        -- fetch the next value
        for i = 1, #t.__orderedIndex do
            if t.__orderedIndex[i] == state then
                key = t.__orderedIndex[i + 1]
            end
        end
    end

    if key then
        return key, t[key]
    end

    -- no more value to return, cleanup
    t.__orderedIndex = nil
    return
end

-- Equivalent of the pairs() function on tables, but iterates keys in alphabetical order.
he.ordered_pairs = function (t)
    return _orderedNext, t, nil
end

-- ------------------------------------------------------------------------------------------------
-- Filter stack

local filter_stack = {}

-- Pushes a filter onto the stack
he.filter_push = function (f)
    if f == nil then
        filter { }
    else
        filter(f)
    end
    table.insert(filter_stack, f)
end

-- Pushes a new filter onto the stack that is the combination of the provided filter values and
-- the currently active filter.
he.filter_push_combine = function (f)
    local active_filter = he.filter_get_active()

    if active_filter == nil then
        he.filter_push(f)
    else
        local new_filter = table.join(active_filter, f)
        he.filter_push(new_filter)
    end
end

-- Pops the active filter off the stack and restores the state to the next filter
he.filter_pop = function ()
    table.remove(filter_stack)
    local prev_filter = he.filter_get_active()
    if prev_filter == nil then
        filter { }
    else
        filter(prev_filter)
    end
end

-- Gets the currently active filter on the stack (possibly nil)
he.filter_get_active = function ()
    return filter_stack[#filter_stack]
end
