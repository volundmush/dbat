local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "remove",
  aliases = { {"remove", 3} },
  min_position = POS.RESTING,
}
