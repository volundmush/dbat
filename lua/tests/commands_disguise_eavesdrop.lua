local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

local DIRS = dbat.consts.directions

local function fixture_exit()
  for vnum = 1, 99 do
    local room = dbat.rooms.by_id(vnum)
    if room then
      for _, dir in pairs(DIRS) do
        local exit = room:exit_get(dir)
        if exit and exit:valid() and exit:destination() then
          return room, dir, exit:destination()
        end
      end
    end
  end
  return nil
end

test:case("disguise and eavesdrop are registered as Lua commands", function(t)
  t:assert(dbat.characters.registry.commands.disguise ~= nil, "disguise command should be registered")
  t:assert(dbat.characters.registry.commands.eavesdrop ~= nil, "eavesdrop command should be registered")
end)

test:case("disguise alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("disguise", commands), true)
  ch:extract()
end)

test:case("eavesdrop can set and clear target room and direction", function(t)
  local room, dir, dest = fixture_exit()
  t:assert(room ~= nil, "test fixtures should include at least one usable exit")

  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  ch:skill_base_set("eavesdrop", 100)

  local dir_name = dbat.consts.direction_names[dir + 1]
  t:eq(ch:execute_command("eavesdrop " .. dir_name, commands), true)
  t:eq(ch:eavesdrop_get(), dest:vnum_get())
  t:eq(ch:eavesdrop_dir_get(), dir)

  t:eq(ch:execute_command("eavesdrop", commands), true)
  t:eq(ch:eavesdrop_get(), 0)
  t:eq(ch:eavesdrop_dir_get(), -1)

  ch:extract()
end)

return test:run()
