local dbat = require("dbat")
dbat.lib = require("lua.lib")
dbat._category_to_namespace = {}
dbat._category_classes = {}

local function get_category_class(namespace, category)
  local key = namespace .. "/" .. category
  local cached = dbat._category_classes[key]
  if cached ~= nil then return cached end
  local mod_path = "lua." .. namespace .. "." .. category
  local ok, result = pcall(require, mod_path)
  local cls = (ok and type(result) == "table" and type(result.wrap) == "function") and result or false
  dbat._category_classes[key] = cls
  return cls
end

function dbat._register(namespace, category, slug, path, value)
  if value == nil then
    error(path .. " returned nil")
  end

  if not dbat[namespace] then
    dbat[namespace] = {}
  end
  if not dbat[namespace].registry then
    dbat[namespace].registry = {}
  end

  dbat._category_to_namespace[category] = namespace

  local ns = dbat[namespace]
  local bucket = ns.registry[category]
  if bucket == nil then
    bucket = {}
    ns.registry[category] = bucket
  end

  if type(value) == "table" then
    value.id = value.id or slug
    value._path = value._path or path
    value._category = value._category or category
    value._namespace = value._namespace or namespace
  end

  if bucket[slug] ~= nil then
    error("duplicate lua entry: " .. namespace .. "/" .. category .. "/" .. slug)
  end

  local cls = get_category_class(namespace, category)
  if cls then value = cls.wrap(value) or value end

  bucket[slug] = value
  return value
end

function dbat.get(category, slug)
  local ns_name = dbat._category_to_namespace[category]
  if not ns_name then return nil end
  local bucket = dbat[ns_name].registry[category]
  if not bucket then return nil end
  return bucket[slug]
end

function dbat.category(category)
  local ns_name = dbat._category_to_namespace[category]
  if not ns_name then return {} end
  return dbat[ns_name].registry[category] or {}
end

function dbat._values(list)
  local index = 0
  return function()
    index = index + 1
    return list[index]
  end
end

-- Lua dispatch entry points called from Zig (char_pcommand_try / char_command_try).
-- Wired onto dbat.characters after Zig has initialised that table.
function dbat.characters.pcommand_try(ch, full_input)
    local cls = require("lua.characters.pcommands")
    return ch:execute_command(full_input, cls)
end

function dbat.characters.command_try(ch, cmd_word, arguments)
    local cls = require("lua.characters.commands")
    local input = (arguments ~= nil and arguments ~= "")
                  and (cmd_word .. " " .. arguments) or cmd_word
    return ch:execute_command(input, cls)
end
