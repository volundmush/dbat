local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "norepeat", aliases = { { "norepeat", 8 } }, flag = T.PRF.NOREPEAT, off = "You will now have your communication repeated.\r\n", on = "You will no longer have your communication repeated.\r\n" }
