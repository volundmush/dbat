local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "sac",
  aliases = { {"sac", 3}, {"sacrifice", 3} },
  min_position = POS.RESTING,
  legacy = "sac",
}
