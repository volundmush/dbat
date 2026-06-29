local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nomusic", aliases = { { "nomusic", 6 } }, flag = T.PRF.NOMUSIC, off = "You will now listen to the music channel.\r\n", on = "You will no longer listen to the music channel.\r\n" }
