local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "holylight", aliases = { { "holylight", 4 } }, flag = T.PRF.HOLYLIGHT, admin_level = T.ADMLVL.IMMORT, off = "HolyLight mode off.\r\n", on = "HolyLight mode on.\r\n" }
