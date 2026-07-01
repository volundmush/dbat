local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "grab",
  aliases = { {"grab", 4}, {"hold", 4} },
  min_position = POS.RESTING,
  legacy = "grab",
}
