local dbat = require("dbat")

local Condition = {}
Condition.__index = Condition

function Condition.wrap(def)
  def.tags           = def.tags           or {}
  def.exclusive_tags = def.exclusive_tags or {}
  def.persistent     = def.persistent     ~= nil and def.persistent     or false
  def.stackable      = def.stackable      ~= nil and def.stackable      or false
  -- A condition may embed a derived stat definition. Register it into the
  -- derived registry so ch:der_total() can find it without a separate file.
  if def.derived then
    local d = def.derived
    d.id = d.id or def.id
    local bucket = dbat.characters.registry["derived"]
    if not bucket then
      bucket = {}
      dbat.characters.registry["derived"] = bucket
    end
    if not bucket[d.id] then bucket[d.id] = d end
  end
  return setmetatable(def, Condition)
end

function Condition:has_tag(tag)
  for _, t in ipairs(self.tags) do
    if t == tag then return true end
  end
  return false
end

function Condition:apply_modifiers(ch, instance)
  if not self.modifiers then return {} end
  return self.modifiers(ch, instance) or {}
end

function Condition:dispatch_event(ch, instance, event)
  if self.on_event then
    self.on_event(ch, instance, event)
  end
  if event == "expire" and ch:condition_has(self.id) then
    ch:condition_remove(self.id, "expired")
  end
end

return Condition
