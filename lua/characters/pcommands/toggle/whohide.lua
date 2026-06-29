local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "whohide", aliases = { { "whohide", 7 } }, flag = T.PRF.HIDE, off = "You are no longer hidden from view on the who list and public channels.\r\n", on = "You are now hidden from view on the who list and public channels.\r\n" }
