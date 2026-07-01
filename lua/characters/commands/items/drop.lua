local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "drop",
  aliases = { {"drop", 3} },
  min_position = POS.RESTING,
}
