local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

local cases = {
  { id = "donate",  input = "donate" },
  { id = "drop",    input = "drop" },
  { id = "fill",    input = "fill" },
  { id = "get",     input = "get" },
  { id = "get",     input = "take" },
  { id = "give",    input = "give" },
  { id = "grab",    input = "grab" },
  { id = "grab",    input = "hold" },
  { id = "junk",    input = "junk" },
  { id = "pour",    input = "pour" },
  { id = "put",     input = "put" },
  { id = "refuel",  input = "refuel" },
  { id = "remove",  input = "remove" },
  { id = "sac",     input = "sac" },
  { id = "sac",     input = "sacrifice" },
  { id = "twohand", input = "twohand" },
  { id = "wear",    input = "wear" },
  { id = "wield",   input = "wield" },
}

test:case("item commands are registered as Lua commands", function(t)
  for _, case in ipairs(cases) do
    t:assert(dbat.characters.registry.commands[case.id] ~= nil, case.id .. " command should be registered")
  end
end)

test:case("item command aliases dispatch through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  ch:position_set(dbat.consts.positions.STANDING)

  for _, case in ipairs(cases) do
    t:eq(ch:execute_command(case.input, commands), true)
  end

  ch:extract()
end)

return test:run()
