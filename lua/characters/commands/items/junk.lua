local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "junk",
  aliases = { {"junk", 4} },
  min_position = POS.RESTING,
}
