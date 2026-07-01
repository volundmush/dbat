local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "pour",
  aliases = { {"pour", 4} },
  min_position = POS.STANDING,
}
