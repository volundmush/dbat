local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

local function sorted_mimicable_races()
  local races = {}
  for _, race in pairs(dbat.characters.registry.races) do
    if race.pc_ok and race.mimic_ok ~= false then
      races[#races + 1] = race
    end
  end
  table.sort(races, function(left, right)
    return left.legacy_id < right.legacy_id
  end)
  return races
end

test:case("mimic is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.mimic
  t:assert(def ~= nil, "mimic command should be registered")
  t:eq(def.id, "mimic")
end)

test:case("mimic alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("mimi", commands), true)
  ch:extract()
end)

test:case("mimic race metadata mirrors legacy policy", function(t)
  local race = dbat.characters.registry.races

  t:eq(race.saiyan.mimic_ok, true)
  t:eq(race.human.mimic_ok, true)
  t:eq(race.android.mimic_ok, true)

  t:eq(race.icer.mimic_ok, false)
  t:eq(race.namekian.mimic_ok, false)
  t:eq(race.bio.mimic_ok, false)
  t:eq(race.demon.mimic_ok, false)
  t:eq(race.majin.mimic_ok, false)
  t:eq(race.hoshijin.mimic_ok, false)
  t:eq(race.arlian.mimic_ok, false)
end)

test:case("mimic race selection can use partial match helper", function(t)
  local utils = dbat.lib.utils
  local match = utils.partial_match(sorted_mimicable_races(), "sai", {
    str_func = function(race)
      return race.name
    end,
  })

  t:assert(match ~= nil, "sai should partially match Saiyan")
  t:eq(match.id, "saiyan")
end)

test:case("mimic binding stores legacy race id", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  t:eq(ch:mimic_set(dbat.characters.registry.races.saiyan.legacy_id), 1)
  t:eq(ch:mimic_get(), 1)
  t:eq(ch:mimic_set(0), 0)
  t:eq(ch:mimic_get(), 0)

  ch:extract()
end)

return test:run()
