local M = {}

M.placeholders = {}
M.functions = {}

local function is_same(left, right)
    if left == nil or right == nil then return false end
    if type(left) == "userdata" and type(right) == "userdata" and left.is_same ~= nil then
        local ok, same = pcall(function() return left:is_same(right) end)
        return ok and same or false
    end
    return left == right
end

local function safe_call(value, method)
    if value == nil or value[method] == nil then return nil end
    local ok, result = pcall(function() return value[method](value) end)
    if ok then return result end
    return nil
end

local function object_text(value)
    if value == nil then return nil end
    if type(value) == "string" then return value end
    if type(value) == "number" or type(value) == "boolean" then return tostring(value) end

    return safe_call(value, "name_get")
        or safe_call(value, "short_description_get")
        or safe_call(value, "title_get")
        or tostring(value)
        or "<error>"
end

local function entity_name(value, viewer)
    if value == nil then return "someone" end
    if is_same(value, viewer) then return "you" end
    if type(value) == "userdata" and value.display_name_for ~= nil then
        local ok, result = pcall(function() return value:display_name_for(viewer) end)
        if ok and result then return result end
    end
    return object_text(value) or "<error>"
end

local function pronouns(value, viewer)
    if value ~= nil and is_same(value, viewer) then
        return { subjective = "you", objective = "you", possessive = "your" }
    end

    local sex = 0
    if value ~= nil and value.sex_get ~= nil then
        local ok, result = pcall(function() return value:sex_get() end)
        if ok then sex = result end
    end

    if sex == 1 then return { subjective = "he", objective = "him", possessive = "his" } end
    if sex == 2 then return { subjective = "she", objective = "her", possessive = "her" } end
    return { subjective = "it", objective = "it", possessive = "its" }
end

local function context_value(context, name)
    if context == nil then return nil end
    if name == "actor" then return context.actor or context.ch end
    if name == "target" then return context.target or context.victim end
    if name == "tool" then return context.tool or context.object or context.obj end
    return context[name]
end

local function render_context_value(viewer, context, name)
    return object_text(context_value(context, name)) or "<error>"
end

local function register_legacy_placeholders()
    M.placeholders["n"] = function(viewer, context) return entity_name(context_value(context, "actor"), viewer) end
    M.placeholders["N"] = function(viewer, context) return entity_name(context_value(context, "target"), viewer) end
    M.placeholders["p"] = function(viewer, context) return entity_name(context_value(context, "tool"), viewer) end
    M.placeholders["P"] = M.placeholders["p"]
    M.placeholders["e"] = function(viewer, context) return pronouns(context_value(context, "actor"), viewer).subjective end
    M.placeholders["m"] = function(viewer, context) return pronouns(context_value(context, "actor"), viewer).objective end
    M.placeholders["s"] = function(viewer, context) return pronouns(context_value(context, "actor"), viewer).possessive end
    M.placeholders["E"] = function(viewer, context) return pronouns(context_value(context, "target"), viewer).subjective end
    M.placeholders["M"] = function(viewer, context) return pronouns(context_value(context, "target"), viewer).objective end
    M.placeholders["S"] = function(viewer, context) return pronouns(context_value(context, "target"), viewer).possessive end
    M.placeholders["$"] = function() return "$" end
end

register_legacy_placeholders()

local function render_function(viewer, context, name, args)
    local handler = M.functions[name]
    if handler == nil then return "<error>" end
    local parsed = {}
    for arg in tostring(args or ""):gmatch("([^,]+)") do
        parsed[#parsed + 1] = (arg:gsub("^%s+", ""):gsub("%s+$", ""))
    end
    local ok, result = pcall(handler, viewer, context, parsed)
    if not ok then return "<error>" end
    return object_text(result) or "<error>"
end

local function render_placeholder(viewer, context, key)
    local handler = M.placeholders[key]
    if handler ~= nil then
        local ok, result = pcall(handler, viewer, context)
        if ok then return object_text(result) or "<error>" end
        return "<error>"
    end

    return render_context_value(viewer, context, key)
end

function M.render_for(viewer, template, context)
    local text = tostring(template or "")
    -- If text doesn't end in a \r\n, add one so that functions can assume it exists without worrying about edge cases.
    if not text:match("\r\n$") then text = text .. "\r\n" end
    text = text:gsub("%$([%a_][%w_]*)%(([^()]*)%)", function(name, args)
        return render_function(viewer, context, name, args)
    end)
    text = text:gsub("%$([%a_][%w_]*)", function(key)
        return render_placeholder(viewer, context, key)
    end)
    text = text:gsub("%$([nNemsEMSpP$])", function(key)
        return render_placeholder(viewer, context, key)
    end)
    return text
end

function M.render(template, context)
    return M.render_for(nil, template, context)
end

local function excluded(ch, list)
    if ch == nil or list == nil then return false end
    for _, item in ipairs(list) do
        if is_same(ch, item) then return true end
    end
    return false
end

function M.to_char(ch, message, context)
    if ch == nil then return end
    ch:send(M.render_for(ch, message, context))
end

function M.to_actor(message, context)
    M.to_char(context and context_value(context, "actor"), message, context)
end

function M.to_target(message, context)
    M.to_char(context and context_value(context, "target"), message, context)
end

function M.to_room(message, context)
    context = context or {}
    hide_invisible = context.hide_invisible or false
    local actor = context_value(context, "actor")
    local room = context.room or (actor and actor:room_get())
    if room == nil then return end

    for ch in room:people() do
        if not excluded(ch, context.exclude) and (not hide_invisible or ch:can_see(actor)) then
            M.to_char(ch, message, context)
        end
    end
end

function M.around(ch, message, context)
    context = context or {}
    context.actor = context.actor or ch
    context.room = context.room or (ch and ch:room_get())
    context.exclude = context.exclude or { ch }
    M.to_room(message, context)
end

function M.message(messages, context)
    context = context or {}
    if type(messages) ~= "table" then return M.to_room(messages, context) end
    if messages.actor then M.to_actor(messages.actor, context) end
    if messages.target then M.to_target(messages.target, context) end
    if messages.room then
        local exclude = context.exclude or {}
        local actor = context_value(context, "actor")
        local target = context_value(context, "target")
        if actor ~= nil then exclude[#exclude + 1] = actor end
        if target ~= nil then exclude[#exclude + 1] = target end
        context.exclude = exclude
        M.to_room(messages.room, context)
    end
end

return M
