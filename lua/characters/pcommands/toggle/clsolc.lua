local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "clsolc", aliases = { { "clsolc", 6 } }, flag = T.PRF.CLS, admin_level = T.ADMLVL.BUILDER, off = "Will no longer clear screen in OLC.\r\n", on = "Will now clear screen in OLC.\r\n" }
