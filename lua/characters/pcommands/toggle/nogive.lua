local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nogive", aliases = { { "nogive", 5 } }, flag = T.PRF.NOGIVE, off = "You will now accept things being given to you.\r\n", on = "You will no longer accept things being given to you.\r\n" }
