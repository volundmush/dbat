local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "roomflags", aliases = { { "roomflags", 5 } }, flag = T.PRF.ROOMFLAGS, admin_level = T.ADMLVL.IMMORT, off = "You will no longer see the room flags.\r\n", on = "You will now see the room flags.\r\n" }
