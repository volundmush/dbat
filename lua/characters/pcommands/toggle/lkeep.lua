local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "lkeep", aliases = { { "lkeep", 4 } }, flag = T.PRF.LKEEP, off = "You will no longer keep cybernetic limbs with death.\r\n", on = "You will now keep cybernetic limbs with death.\r\n" }
