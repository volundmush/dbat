local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "brief", aliases = { { "brief", 2 } }, flag = T.PRF.BRIEF, off = "Brief mode off.\r\n", on = "Brief mode on.\r\n" }
