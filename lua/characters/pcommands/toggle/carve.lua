local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "carve", aliases = { { "carve", 4 } }, flag = T.PRF.CARVE, off = "You will no longer worry about acquiring steaks from animals.\r\n", on = "You will now acquire steaks from animal if you can.\r\n" }
