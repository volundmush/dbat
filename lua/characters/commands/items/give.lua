local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "give",
  aliases = { {"give", 3} },
  min_position = POS.RESTING,
}
