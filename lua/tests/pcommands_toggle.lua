local test = require("lua.test").new()
local dbat = require("dbat")
local pcommands = require("lua.characters.pcommands")

local toggle_commands = {
  afk = "afk",
  autogold = "autogold",
  autoloot = "autoloot",
  autosplit = "autosplit",
  brief = "brief",
  buildwalk = "buildwalk",
  carve = "carve",
  clsolc = "clsolc",
  compact = "compact",
  health = "health",
  hide = "hide",
  holylight = "holylight",
  ihealth = "ihealth",
  instruct = "instruct",
  lkeep = "lkeep",
  nocompress = "nocompress",
  noeq = "noeq",
  nolin = "nolin",
  nomusic = "nomusic",
  noooc = "noooc",
  nogive = "nogive",
  nograts = "nograts",
  nohassle = "nohassle",
  nomail = "nomail",
  nonewbie = "nonewbie",
  noparry = "noparry",
  norepeat = "norepeat",
  noshout = "noshout",
  nosummon = "nosummon",
  notell = "notell",
  nowiz = "nowiz",
  roomflags = "roomflags",
  slowns = "slowns",
  sneak = "sneak",
  test = "test",
  trackthru = "trackthru",
  vieworder = "vieworder",
  whohide = "whohide",
}

test:case("toggle pcommands are registered individually", function(t)
  for slug, id in pairs(toggle_commands) do
    local def = dbat.characters.registry.pcommands[slug]
    t:assert(def ~= nil, slug .. " pcommand should be registered")
    t:eq(def.id, id)
  end
end)

test:case("toggle pcommands claim their command words through Lua matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  t:eq(ch:execute_command("brief", pcommands), true)
  t:eq(ch:execute_command("hide", pcommands), true)
  t:eq(ch:execute_command("nocompress", pcommands), true)

  ch:extract()
end)

test:case("toggle config bindings are exposed", function(t)
  t:eq(type(dbat.config.compression_enabled()), "boolean")
  t:eq(type(dbat.config.nameserver_slow_toggle()), "boolean")
  t:eq(type(dbat.config.nameserver_slow_toggle()), "boolean")
  t:eq(type(dbat.config.track_through_doors_toggle()), "boolean")
  t:eq(type(dbat.config.track_through_doors_toggle()), "boolean")
end)

return test:run()
