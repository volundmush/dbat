#include "command.h"

#include "comm.h"
#include "interpreter.h"
#include "random.h"

#include "act.comm.h"
#include "act.informative.h"
#include "act.item.h"
#include "act.misc.h"
#include "act.other.h"
#include "act.social.h"
#include "act.wizard.h"
#include "aedit.h"
#include "alias.h"
#include "assedit.h"
#include "ban.h"
#include "db.h"
#include "dg_scripts.h"
#include "disabled.h"
#include "graph.h"
#include "guild.h"
#include "hedit.h"
#include "house.h"
#include "modify.h"
#include "oasis.h"
#include "oasis_copy.h"

#include "tedit.h"
#include "vehicles.h"

#include "consts/admlevel.h"
#include "consts/affflags.h"
#include "consts/positions.h"

#include "iterate.hpp"

#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_utils.h"
#include "character_macros.h"
#include "consts/affflags.h"
#include "consts/itemdata.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "flags.h"
#include "object_db.h"
#include "object_impl.h"
#include "object_macros.h"
#include "room_api.h"

#include <cctype>
#include <cstring>

const struct command_info cmd_info[] = {
    {"RESERVED", "", 0, 0, 0, ADMLVL_NONE,
     0}, /* this must be first -- for specprocs */

    /* directions: handled by Lua move command */

    /* now, the main list */
    /* do_absorb moved to lua/characters/commands/misc/absorb.lua */
    {"at", "at", POS_DEAD, do_at, 0, ADMLVL_BUILDER, 0},
    {"advance", "adv", POS_DEAD, do_advance, 0, ADMLVL_GRGOD, 0},
    {"aedit", "aed", POS_DEAD, do_oasis, 0, ADMLVL_BUILDER, SCMD_OASIS_AEDIT},
    {"alias", "ali", POS_DEAD, do_alias, 0, ADMLVL_NONE, 0},
    /* afk moved to lua/characters/pcommands/toggle/afk.lua */
    // {"aid", "aid", POS_STANDING, do_aid, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/aid.lua
    /* do_appraise moved to lua/characters/commands/misc/appraise.lua */
    {"approve", "approve", POS_STANDING, do_approve, 0, ADMLVL_IMMORT, 0},
    /* do_arena moved to lua/characters/commands/misc/arena.lua */
    /* ashcloud moved to lua/characters/commands/misc/ashcloud.lua */
    {"assedit", "assed", POS_STANDING, do_assedit, 0, ADMLVL_GOD, 0},
    {"astat", "ast", POS_DEAD, do_astat, 0, ADMLVL_GOD, 0},
    {"ask", "ask", POS_RESTING, do_spec_comm, 0, ADMLVL_NONE, SCMD_ASK},
    {"auction", "auctio", POS_RESTING, do_not_here, 0, 0, 0},
    {"augment", "augmen", POS_SITTING, do_not_here, 1, ADMLVL_NONE, 0},
    // {"aura", "aura", POS_RESTING, do_aura, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/aura.lua
    {"autoexit", "autoex", POS_DEAD, do_autoexit, 0, ADMLVL_NONE, 0},
    /* autogold moved to lua/characters/pcommands/toggle/autogold.lua */
    /* autoloot moved to lua/characters/pcommands/toggle/autoloot.lua */
    /* autosplit moved to lua/characters/pcommands/toggle/autosplit.lua */

    {"ban", "ban", POS_DEAD, do_ban, 0, ADMLVL_VICE, 0},
    {"balance", "bal", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"bid", "bi", POS_RESTING, do_bid, 0, 0, 0},
    {"book", "boo", POS_SLEEPING, do_gen_ps, 0, ADMLVL_IMMORT, SCMD_INFO},
    /* do_break moved to lua/characters/commands/misc/break.lua */
    /* brief moved to lua/characters/pcommands/toggle/brief.lua */
    {"build", "bui", POS_SITTING, do_assemble, 0, ADMLVL_NONE, SCMD_BREW},
    /* buildwalk moved to lua/characters/pcommands/toggle/buildwalk.lua */
    {"buy", "bu", POS_STANDING, do_not_here, 0, ADMLVL_NONE, 0},
    {"bug", "bug", POS_DEAD, do_gen_write, 0, ADMLVL_NONE, SCMD_BUG},

    {"cancel", "cance", POS_RESTING, do_not_here, 0, 0, 0},
    // {"candy", "cand", POS_FIGHTING, do_candy, 0, 0, 0},  // lua/characters/commands/misc/candy.lua
    /* carve moved to lua/characters/pcommands/toggle/carve.lua */
    {"cedit", "cedit", POS_DEAD, do_oasis, 0, ADMLVL_IMPL, SCMD_OASIS_CEDIT},
    /* do_channel moved to lua/characters/commands/misc/channel.lua */
    {"check", "ch", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"checkload", "checkl", POS_DEAD, do_checkloadstatus, 0, ADMLVL_GOD, 0},
    {"chown", "cho", POS_DEAD, do_chown, 1, ADMLVL_IMPL, 0},
    {"clear", "cle", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_CLEAR},
    {"cls", "cls", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_CLEAR},
    /* clsolc moved to lua/characters/pcommands/toggle/clsolc.lua */
    {"color", "col", POS_DEAD, do_color, 0, ADMLVL_NONE, 0},
/* do_compare moved to lua/characters/pcommands/misc/compare.lua */
    {"commands", "com", POS_DEAD, do_commands, 0, ADMLVL_NONE, SCMD_COMMANDS},
    /* do_commune moved to lua/characters/commands/misc/commune.lua */
    /* compact moved to lua/characters/pcommands/toggle/compact.lua */
    /* cook moved to lua/characters/commands/misc/cook.lua */
    {"copyover", "copyover", POS_DEAD, do_copyover, 0, ADMLVL_GOD, 0},
    {"credits", "cred", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_CREDITS},

    {"date", "da", POS_DEAD, do_date, 0, ADMLVL_IMMORT, SCMD_DATE},
    {"dc", "dc", POS_DEAD, do_dc, 0, ADMLVL_GOD, 0},
    /* decapitate → lua/characters/commands/misc/spoil.lua */
    {"deploy", "deplo", POS_STANDING, do_deploy, 0, ADMLVL_NONE, 0},
    {"deposit", "depo", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"diagnose", "diagnos", POS_RESTING, do_diagnose, 0, ADMLVL_NONE, 0},
    {"disable", "disa", POS_DEAD, do_disable, 0, ADMLVL_VICE, 0},
    /* do_disguise moved to lua/characters/commands/misc/disguise.lua */
    /* do_bury/dig moved to lua/characters/commands/misc/bury.lua */
    /* do_display moved to lua/characters/pcommands/misc/display.lua */
    {"donate", "don", POS_RESTING, do_drop, 0, ADMLVL_NONE, SCMD_DONATE},
    // {"drag", "dra", POS_STANDING, do_drag, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/drag.lua
    {"drop", "dro", POS_RESTING, do_drop, 0, ADMLVL_NONE, SCMD_DROP},
    {"dub", "du", POS_STANDING, do_intro, 0, ADMLVL_NONE, 0},
    /* do_eavesdrop moved to lua/characters/commands/misc/eavesdrop.lua */
    {"echo", "ec", POS_SLEEPING, do_echo, 0, ADMLVL_IMMORT, SCMD_ECHO},
    {"emote", "em", POS_RESTING, do_echo, 1, ADMLVL_NONE, SCMD_EMOTE},
    {":", ":", POS_RESTING, do_echo, 1, ADMLVL_NONE, SCMD_EMOTE},
    /* ensnare → lua/characters/commands/misc/ensnare.lua */
    /* escape → lua/characters/commands/misc/escape.lua */
    {"exchange", "exchan", POS_RESTING, do_rptrans, 0, ADMLVL_NONE, 0},
    {"exits", "ex", POS_RESTING, do_exits, 0, ADMLVL_NONE, 0},
    {"examine", "exa", POS_SITTING, do_examine, 0, ADMLVL_NONE, 0},

    /* feed → lua/characters/commands/misc/feed.lua */
    {"fill", "fil", POS_STANDING, do_pour, 0, ADMLVL_NONE, SCMD_FILL},
    {"file", "fi", POS_SLEEPING, do_file, 0, ADMLVL_IMMORT, 0},
    {"finddoor", "findd", POS_SLEEPING, do_finddoor, 0, ADMLVL_IMMORT, 0},
    {"findkey", "findk", POS_SLEEPING, do_findkey, 0, ADMLVL_IMMORT, 0},
    {"finger", "finge", POS_SLEEPING, do_finger, 0, ADMLVL_NONE, 0},
    /* do_fireshield moved to lua/characters/commands/misc/fireshield.lua */
    /* do_fix moved to lua/characters/commands/misc/fix.lua */
    /* do_focus moved to lua/characters/commands/misc/focus.lua */
    {"force", "force", POS_SLEEPING, do_force, 0, ADMLVL_IMMORT, 0},
    /* do_forgery moved to lua/characters/commands/misc/forgery.lua */
    {"forget", "forg", POS_RESTING, do_not_here, 0, ADMLVL_NONE, 0},
    {"freeze", "freeze", POS_DEAD, do_wizutil, 0, ADMLVL_IMMORT, SCMD_FREEZE},
    // {"fury", "fury", POS_FIGHTING, do_fury, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/fury.lua
    // {"future", "futu", POS_STANDING, do_future, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/future.lua

    {"gain", "ga", POS_RESTING, do_not_here, 0, ADMLVL_NONE, 0},
    {"game", "gam", POS_RESTING, do_show, 0, ADMLVL_IMMORT, 0},
    {"get", "get", POS_RESTING, do_get, 0, ADMLVL_NONE, 0},
    {"gecho", "gecho", POS_DEAD, do_gecho, 0, ADMLVL_BUILDER, 0},
    {"gedit", "gedit", POS_DEAD, do_oasis, 0, ADMLVL_BUILDER, SCMD_OASIS_GEDIT},
    {"gemote", "gem", POS_SLEEPING, do_gen_comm, 0, ADMLVL_NONE, SCMD_GEMOTE},
    {"generator", "genr", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"glist", "glist", POS_SLEEPING, do_oasis, 0, ADMLVL_BUILDER,
     SCMD_OASIS_GLIST},
    {"give", "giv", POS_RESTING, do_give, 0, ADMLVL_NONE, 0},
    /* do_group moved to lua/characters/commands/misc/group.lua */
    {"grab", "grab", POS_RESTING, do_grab, 0, ADMLVL_NONE, 0},
    {"grand", "gran", POS_RESTING, do_not_here, 0, ADMLVL_NONE, 0},
    /* do_grapple moved to lua/characters/commands/misc/grapple.lua */
    {"grats", "grat", POS_SLEEPING, do_gen_comm, 0, ADMLVL_NONE, SCMD_GRATZ},
    {"gravity", "grav", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"gsay", "gsay", POS_SLEEPING, do_gsay, 0, ADMLVL_NONE, 0},
    {"gtell", "gt", POS_SLEEPING, do_gsay, 0, ADMLVL_NONE, 0},
    {"handout", "hand", POS_STANDING, do_handout, 0, ADMLVL_GOD, 0},
    // {"hasshuken", "hasshuke", POS_STANDING, do_hass, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/hass.lua
    /* health moved to lua/characters/pcommands/toggle/health.lua */
    {"help", "h", POS_DEAD, do_help, 0, ADMLVL_NONE, 0},
    {"hedit", "hedit", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_HEDIT},
    {"hindex", "hind", POS_DEAD, do_hindex, 0, ADMLVL_NONE, 0},
    {"helpcheck", "helpch", POS_DEAD, do_helpcheck, 0, ADMLVL_NONE, 0},
    {"handbook", "handb", POS_DEAD, do_gen_ps, 0, ADMLVL_IMMORT, SCMD_HANDBOOK},
    /* hide moved to lua/characters/pcommands/toggle/hide.lua */
    {"history", "hist", POS_DEAD, do_history, 0, ADMLVL_NONE, 0},
    {"hold", "hold", POS_RESTING, do_grab, 1, ADMLVL_NONE, 0},
    /* holylight moved to lua/characters/pcommands/toggle/holylight.lua */
    {"house", "house", POS_RESTING, do_house, 0, ADMLVL_NONE, 0},
    {"hsedit", "hsedit", POS_DEAD, do_oasis, 0, ADMLVL_BUILDER,
     SCMD_OASIS_HSEDIT},
    {"htank", "htan", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    /* hyoga → lua/characters/commands/misc/obstruct.lua */

    /* ihealth moved to lua/characters/pcommands/toggle/ihealth.lua */
    {"info", "info", POS_DEAD, do_ginfo, 0, ADMLVL_IMMORT, 0},
    /* {"ingest", "inges", POS_STANDING, do_ingest, 0, ADMLVL_NONE, 0}, Lua */
    {"imotd", "imotd", POS_DEAD, do_gen_ps, 0, ADMLVL_IMMORT, SCMD_IMOTD},
    {"immlist", "imm", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_WIZLIST},
    /* do_implant moved to lua/characters/commands/misc/implant.lua */
    /* do_instill moved to lua/characters/commands/misc/instill.lua */
    /* instruct moved to lua/characters/pcommands/toggle/instruct.lua */
    {"interest", "inter", POS_DEAD, do_interest, 0, ADMLVL_IMPL, 0},
    {"iedit", "ie", POS_DEAD, do_iedit, 0, ADMLVL_IMPL, 0},
    {"invis", "invi", POS_DEAD, do_invis, 0, ADMLVL_IMMORT, 0},
    // {"iwarp", "iwarp", POS_RESTING, do_warp, 0, ADMLVL_NONE, 0}, // lua/characters/commands/misc/warp.lua

    {"junk", "junk", POS_RESTING, do_drop, 0, ADMLVL_NONE, SCMD_JUNK},

    /* do_kaioken moved to lua/characters/commands/misc/kaioken.lua */
    // {"kuraiiro", "kuraiir", POS_FIGHTING, do_kura, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/kura.lua
    {"look", "lo", POS_RESTING, do_look, 0, ADMLVL_NONE, SCMD_LOOK},
    {"lag", "la", POS_RESTING, do_lag, 0, 5, 0},
    {"languages", "lang", POS_RESTING, do_languages, 0, ADMLVL_NONE, 0},
    {"last", "last", POS_DEAD, do_last, 0, ADMLVL_GOD, 0},
    {"learn", "lear", POS_RESTING, do_not_here, 0, ADMLVL_NONE, 0},
    {"list", "lis", POS_STANDING, do_not_here, 0, ADMLVL_NONE, 0},
    {"links", "lin", POS_DEAD, do_oasis, 0, ADMLVL_BUILDER, SCMD_OASIS_LINKS},
    /* liquefy → lua/characters/commands/misc/liquefy.lua */
    /* lkeep moved to lua/characters/pcommands/toggle/lkeep.lua */
    {"lockout", "lock", POS_STANDING, do_hell, 0, ADMLVL_IMMORT, 0},
    {"lua", "lua", POS_DEAD, do_lua, 0, ADMLVL_IMPL, 0},
    // {"majinize", "majini", POS_STANDING, do_majinize, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/majinize.lua
    {"motd", "motd", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_MOTD},
    {"mail", "mail", POS_STANDING, do_not_here, 2, ADMLVL_NONE, 0},
    {"map", "map", POS_STANDING, do_map, 0, ADMLVL_NONE, 0},
    {"medit", "medit", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_MEDIT},
    /* do_meditate moved to lua/characters/commands/misc/meditate.lua */
    /* do_mimic moved to lua/characters/commands/misc/mimic.lua */
    {"mlist", "mlist", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_MLIST},
    /* do_moondust moved to lua/characters/commands/misc/moondust.lua */
    /* multiform → lua/characters/commands/misc/multiform.lua */
    {"mute", "mute", POS_DEAD, do_wizutil, 0, ADMLVL_IMMORT, SCMD_SQUELCH},
    {"music", "musi", POS_RESTING, do_gen_comm, 1, ADMLVL_NONE, SCMD_HOLLER},
    {"newbie", "newbie", POS_SLEEPING, do_gen_comm, 0, ADMLVL_NONE,
     SCMD_AUCTION},
    {"news", "news", POS_SLEEPING, do_news, 0, ADMLVL_NONE, 0},
    {"newsedit", "newsedi", POS_SLEEPING, do_newsedit, 0, ADMLVL_IMMORT, 0},
    {"nickname", "nicknam", POS_RESTING, do_nickname, 0, ADMLVL_NONE, 0},
    /* nocompress moved to lua/characters/pcommands/toggle/nocompress.lua */
    /* noeq moved to lua/characters/pcommands/toggle/noeq.lua */
    /* nolin moved to lua/characters/pcommands/toggle/nolin.lua */
    /* nomusic moved to lua/characters/pcommands/toggle/nomusic.lua */
    /* noooc moved to lua/characters/pcommands/toggle/noooc.lua */
    /* nogive moved to lua/characters/pcommands/toggle/nogive.lua */
    /* nograts moved to lua/characters/pcommands/toggle/nograts.lua */
    /* nohassle moved to lua/characters/pcommands/toggle/nohassle.lua */
    /* nomail moved to lua/characters/pcommands/toggle/nomail.lua */
    /* nonewbie moved to lua/characters/pcommands/toggle/nonewbie.lua */
    /* noparry moved to lua/characters/pcommands/toggle/noparry.lua */
    /* norepeat moved to lua/characters/pcommands/toggle/norepeat.lua */
    /* noshout moved to lua/characters/pcommands/toggle/noshout.lua */
    /* nosummon moved to lua/characters/pcommands/toggle/nosummon.lua */
    /* notell moved to lua/characters/pcommands/toggle/notell.lua */
    {"notitle", "notitle", POS_DEAD, do_wizutil, 0, ADMLVL_GOD, SCMD_NOTITLE},
    /* nowiz moved to lua/characters/pcommands/toggle/nowiz.lua */
    {"ooc", "ooc", POS_SLEEPING, do_gen_comm, 0, ADMLVL_NONE, SCMD_GOSSIP},
    {"offer", "off", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"olc", "olc", POS_DEAD, do_show_save_list, 0, ADMLVL_IMMORT, 0},
    {"olist", "olist", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_OLIST},
    {"oedit", "oedit", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_OEDIT},
    {"pack", "pac", POS_STANDING, do_pack, 0, 0, 0},
    {"page", "pag", POS_DEAD, do_page, 0, ADMLVL_BUILDER, 0},
    // {"paralyze", "paralyz", POS_FIGHTING, do_paralyze, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/paralyze.lua
    {"peace", "pea", POS_DEAD, do_peace, 0, ADMLVL_BUILDER, 0},
    {"perfect", "perfec", POS_DEAD, do_perf, 0, ADMLVL_NONE, 0},
    {"permission", "permiss", POS_DEAD, do_permission, 0, ADMLVL_IMMORT, 0},
    {"pickup", "picku", POS_RESTING, do_not_here, 0, 0, 0},
    // {"pilot", "pilot", POS_SITTING, do_drive, 0, ADMLVL_NONE, 0}, // lua/characters/commands/misc/drive.lua
    /* do_plant moved to lua/characters/commands/misc/plant.lua */
    {"players", "play", POS_DEAD, do_plist, 0, ADMLVL_IMPL, 0},
    {"poofin", "poofi", POS_DEAD, do_poofset, 0, ADMLVL_IMMORT, SCMD_POOFIN},
    {"poofout", "poofo", POS_DEAD, do_poofset, 0, ADMLVL_IMMORT, SCMD_POOFOUT},
    // {"pose", "pos", POS_STANDING, do_pose, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/pose.lua
    {"post", "pos", POS_STANDING, do_post, 0, ADMLVL_NONE, 0},
    // {"potential", "poten", POS_STANDING, do_potential, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/potential.lua
    {"pour", "pour", POS_STANDING, do_pour, 0, ADMLVL_NONE, SCMD_POUR},
    {"program", "progra", POS_DEAD, do_oasis, 0, ADMLVL_NONE, SCMD_OASIS_REDIT},
    /* prompt alias handled by lua/characters/pcommands/misc/display.lua */
    {"practice", "pra", POS_RESTING, do_practice, 1, ADMLVL_NONE, 0},
    /* do_pushup moved to lua/characters/commands/misc/pushup.lua */
    {"put", "put", POS_RESTING, do_put, 0, ADMLVL_NONE, 0},
    {"purge", "purge", POS_DEAD, do_purge, 0, ADMLVL_BUILDER, 0},

    {"qui", "qui", POS_DEAD, do_quit, 0, ADMLVL_NONE, 0},
    {"quit", "quit", POS_DEAD, do_quit, 0, ADMLVL_NONE, SCMD_QUIT},

    {"radar", "rada", POS_RESTING, do_sradar, 0, ADMLVL_NONE, 0},
    {"raise", "rai", POS_DEAD, do_raise, 0, ADMLVL_NONE, 0},
    {"refuel", "refue", POS_SITTING, do_refuel, 0, ADMLVL_NONE, 0},
    /* do_resize moved to lua/characters/commands/misc/resize.lua */
    {"restring", "restring", POS_STANDING, do_restring, 0, ADMLVL_NONE, 0},
    {"rclone", "rclon", POS_DEAD, do_rcopy, 0, ADMLVL_BUILDER, 0},
    {"rcopy", "rcopy", POS_DEAD, do_rcopy, 0, ADMLVL_BUILDER, 0},
    {"roomdisplay", "roomdisplay", POS_RESTING, do_rdisplay, 0, ADMLVL_NONE, 0},
    {"read", "rea", POS_RESTING, do_look, 0, ADMLVL_NONE, SCMD_READ},
    /* do_recharge moved to lua/characters/commands/misc/recharge.lua */
    /* do_regenerate moved to lua/characters/commands/misc/regenerate.lua */
    /* do_srepair moved to lua/characters/commands/misc/repair.lua */
    {"reply", "rep", POS_SLEEPING, do_reply, 0, ADMLVL_NONE, 0},
    {"reward", "rewar", POS_RESTING, do_reward, 0, ADMLVL_IMMORT, 0},
    {"reload", "reload", POS_DEAD, do_reboot, 0, 5, 0},
    {"receive", "rece", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"remove", "rem", POS_RESTING, do_remove, 0, ADMLVL_NONE, 0},
    {"rent", "rent", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"report", "repor", POS_DEAD, do_gen_write, 0, ADMLVL_NONE, SCMD_IDEA},
    {"reroll", "rero", POS_DEAD, do_wizutil, 0, ADMLVL_IMPL, SCMD_REROLL},
    {"respond", "resp", POS_RESTING, do_respond, 1, ADMLVL_NONE, 0},
    {"return", "retu", POS_DEAD, do_return, 0, ADMLVL_NONE, 0},
    {"redit", "redit", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_REDIT},
    // {"rip", "ri", POS_DEAD, do_rip, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/rip.lua
    {"rlist", "rlist", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_RLIST},
    /* roomflags moved to lua/characters/pcommands/toggle/roomflags.lua */
    /* rpp moved to lua/characters/pcommands/misc/rpp.lua */
    {"say", "say", POS_RESTING, do_say, 0, ADMLVL_NONE, 0},
    {"'", "'", POS_RESTING, do_say, 0, ADMLVL_NONE, 0},
    {"save", "sav", POS_SLEEPING, do_save, 0, ADMLVL_NONE, 0},
    {"saveall", "saveall", POS_DEAD, do_saveall, 0, ADMLVL_BUILDER, 0},
    {"scan", "sca", POS_FIGHTING, do_scan, 0, ADMLVL_NONE, 0},
    /* do_scouter moved to lua/characters/commands/misc/scouter.lua */
    /* do_scry moved to lua/characters/commands/misc/scry.lua */
    {"snet", "snet", POS_RESTING, do_snet, 0, ADMLVL_NONE, 0},
    {"search", "sea", POS_STANDING, do_look, 0, ADMLVL_NONE, SCMD_SEARCH},
    {"sell", "sell", POS_STANDING, do_not_here, 0, ADMLVL_NONE, 0},
    {"sedit", "sedit", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_SEDIT},
    {"send", "send", POS_SLEEPING, do_send, 0, ADMLVL_GOD, 0},
    {"sense", "sense", POS_RESTING, do_track, 0, ADMLVL_NONE, 0},
    {"set", "set", POS_DEAD, do_set, 0, ADMLVL_IMMORT, 0},
    {"shout", "sho", POS_RESTING, do_gen_comm, 0, ADMLVL_NONE, SCMD_SHOUT},
    {"shutdow", "shutdow", POS_DEAD, do_shutdown, 0, ADMLVL_IMPL, 0},
    {"shutdown", "shutdown", POS_DEAD, do_shutdown, 0, ADMLVL_IMPL,
     SCMD_SHUTDOWN},
    /* silk moved to lua/characters/commands/misc/silk.lua */
    /* do_situp moved to lua/characters/commands/misc/situp.lua */
    {"skills", "skills", POS_SLEEPING, do_skills, 0, ADMLVL_NONE, 0},
    {"skillset", "skillset", POS_SLEEPING, do_skillset, 0, 5, 0},
    {"slist", "slist", POS_SLEEPING, do_oasis, 0, ADMLVL_IMMORT,
     SCMD_OASIS_SLIST},
    /* slowns moved to lua/characters/pcommands/toggle/slowns.lua */
    {"smote", "sm", POS_RESTING, do_echo, 1, ADMLVL_NONE, SCMD_SMOTE},
    /* sneak moved to lua/characters/pcommands/toggle/sneak.lua */
    {"snoop", "snoop", POS_DEAD, do_snoop, 0, ADMLVL_IMMORT, 0},
    {"socials", "socials", POS_DEAD, do_commands, 0, ADMLVL_NONE, SCMD_SOCIALS},
    /* do_spar moved to lua/characters/commands/misc/spar.lua */
    // {"spit", "spi", POS_STANDING, do_spit, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/spit.lua
    /* do_split moved to lua/characters/commands/misc/split.lua */
    {"speak", "spe", POS_RESTING, do_languages, 0, ADMLVL_NONE, 0},
    {"spells", "spel", POS_RESTING, do_spells, 0, ADMLVL_IMMORT, 0},
    /* do_beacon/stake moved to lua/characters/commands/misc/beacon.lua */
    {"stat", "stat", POS_DEAD, do_stat, 0, ADMLVL_IMMORT, 0},
    /* do_steal moved to lua/characters/commands/misc/steal.lua */
    // {"stone", "ston", POS_STANDING, do_spit, 0, ADMLVL_NONE, 0}, // lua/characters/commands/misc/spit.lua
    // {"stop", "sto", POS_STANDING, do_stop, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/stop.lua
    {"study", "stu", POS_RESTING, do_not_here, 0, ADMLVL_NONE, 0},
    {"summon", "summo", POS_STANDING, do_summon, 0, ADMLVL_NONE, 0},
    // {"suppress", "suppres", POS_STANDING, do_suppress, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/suppress.lua
    /* swallow → lua/characters/commands/misc/swallow.lua */
    {"switch", "switch", POS_DEAD, do_switch, 0, ADMLVL_VICE, 0},
    {"syslog", "syslog", POS_DEAD, do_syslog, 0, ADMLVL_IMMORT, 0},
    {"teach", "teac", POS_STANDING, do_teach, 0, ADMLVL_NONE, 0},
    {"tell", "tel", POS_DEAD, do_tell, 0, ADMLVL_NONE, 0},
    {"take", "tak", POS_RESTING, do_get, 0, ADMLVL_NONE, 0},
    // {"taisha", "taish", POS_FIGHTING, do_taisha, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/taisha.lua
    {"teleport", "tele", POS_DEAD, do_teleport, 0, ADMLVL_IMMORT, 0},
    /* do_telepathy moved to lua/characters/commands/misc/telepathy.lua */
    {"tedit", "tedit", POS_DEAD, do_tedit, 0, ADMLVL_GRGOD, 0},
    /* test moved to lua/characters/pcommands/toggle/test.lua */
    {"thaw", "thaw", POS_DEAD, do_wizutil, 0, ADMLVL_IMMORT, SCMD_THAW},
    /* do_think moved to lua/characters/commands/misc/think.lua */
    {"title", "title", POS_DEAD, do_title, 0, ADMLVL_NONE, 0},
    {"toggle", "toggle", POS_DEAD, do_toggle, 0, ADMLVL_NONE, 0},
    {"toplist", "toplis", POS_DEAD, do_toplist, 0, ADMLVL_NONE, 0},
    /* trackthru moved to lua/characters/pcommands/toggle/trackthru.lua */
    // {"train", "train", POS_STANDING, do_train, 0, ADMLVL_NONE, 0},  // lua/characters/commands/misc/train.lua
    {"transfer", "transfer", POS_SLEEPING, do_trans, 0, ADMLVL_IMMORT, 0},
    {"transo", "trans", POS_STANDING, do_transobj, 0, 5, 0},
    {"trigedit", "trigedit", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT,
     SCMD_OASIS_TRIGEDIT},
    /* do_trip moved to lua/characters/commands/misc/trip.lua */
    {"tunnel", "tunne", POS_DEAD, do_dig, 0, ADMLVL_IMMORT, 0},
    {"twohand", "twohand", POS_DEAD, do_twohand, 0, ADMLVL_NONE, 0},
    {"typo", "typo", POS_DEAD, do_gen_write, 0, ADMLVL_NONE, SCMD_TYPO},

    /* do_ungroup moved to lua/characters/commands/misc/ungroup.lua */
    {"unban", "unban", POS_DEAD, do_unban, 0, ADMLVL_GRGOD, 0},
    {"unaffect", "unaffect", POS_DEAD, do_wizutil, 0, ADMLVL_GOD,
     SCMD_UNAFFECT},
    /* do_upgrade moved to lua/characters/commands/misc/upgrade.lua */
    {"uptime", "uptime", POS_DEAD, do_date, 0, ADMLVL_IMMORT, SCMD_UPTIME},
    {"use", "use", POS_SITTING, do_use, 1, ADMLVL_NONE, SCMD_USE},
    {"users", "users", POS_DEAD, do_users, 0, ADMLVL_IMMORT, 0},

    {"value", "val", POS_STANDING, do_not_here, 0, ADMLVL_NONE, 0},
    {"varstat", "varst", POS_DEAD, do_varstat, 0, ADMLVL_IMMORT, 0},
    {"version", "ver", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_VERSION},
    /* vieworder moved to lua/characters/pcommands/toggle/vieworder.lua */
    {"visible", "vis", POS_RESTING, do_visible, 1, ADMLVL_NONE, 0},
    {"voice", "voic", POS_RESTING, do_voice, 0, ADMLVL_NONE, 0},
    {"vstat", "vstat", POS_DEAD, do_vstat, 0, ADMLVL_IMMORT, 0},
    /* do_warppool moved to lua/characters/commands/misc/warppool.lua */
    {"wear", "wea", POS_RESTING, do_wear, 0, ADMLVL_NONE, 0},
    {"who", "who", POS_DEAD, do_who, 0, ADMLVL_NONE, 0},
    {"whoami", "whoami", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_WHOAMI},
    /* whohide moved to lua/characters/pcommands/toggle/whohide.lua */
    {"whois", "whois", POS_DEAD, do_whois, 0, ADMLVL_NONE, 0},
    {"where", "where", POS_RESTING, do_where, 1, ADMLVL_IMMORT, 0},
    {"whisper", "whisper", POS_RESTING, do_spec_comm, 0, ADMLVL_NONE,
     SCMD_WHISPER},
    {"wield", "wie", POS_RESTING, do_wield, 0, ADMLVL_NONE, 0},
    /* do_willpower moved to lua/characters/commands/misc/willpower.lua */
    /* do_value/SCMD_WIMPY moved to lua/characters/commands/misc/wimpy.lua */
    {"withdraw", "withdraw", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"wire", "wir", POS_STANDING, do_not_here, 1, ADMLVL_NONE, 0},
    {"wiznet", "wiz", POS_DEAD, do_wiznet, 0, ADMLVL_IMMORT, 0},
    {";", ";", POS_DEAD, do_wiznet, 0, ADMLVL_IMMORT, 0},
    {"wizhelp", "wizhelp", POS_SLEEPING, do_commands, 0, ADMLVL_IMMORT,
     SCMD_WIZHELP},
    {"wizlist", "wizlist", POS_DEAD, do_gen_ps, 0, ADMLVL_NONE, SCMD_WIZLIST},
    {"wizlock", "wizlock", POS_DEAD, do_wizlock, 0, ADMLVL_IMMORT, 0},
    {"wizupdate", "wizupdate", POS_DEAD, do_wizupdate, 0, ADMLVL_IMPL, 0},
    {"write", "write", POS_STANDING, do_write, 1, ADMLVL_NONE, 0},

    {"zcheck", "zcheck", POS_DEAD, do_zcheck, 0, ADMLVL_GOD, 0},
    {"zreset", "zreset", POS_DEAD, do_zreset, 0, ADMLVL_IMMORT, 0},
    {"zedit", "zedit", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_ZEDIT},
    {"zlist", "zlist", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_ZLIST},
    {"zpurge", "zpurge", POS_DEAD, do_zpurge, 0, ADMLVL_GRGOD, 0},

    /* DG trigger commands */
    {"attach", "attach", POS_DEAD, do_attach, 0, ADMLVL_BUILDER, 0},
    {"detach", "detach", POS_DEAD, do_detach, 0, ADMLVL_BUILDER, 0},
    {"detect", "detec", POS_STANDING, do_radar, 0, ADMLVL_NONE, 0},
    {"tlist", "tlist", POS_DEAD, do_oasis, 0, ADMLVL_IMMORT, SCMD_OASIS_TLIST},
    {"tstat", "tstat", POS_DEAD, do_tstat, 0, ADMLVL_IMMORT, 0},
    {"masound", "masound", POS_DEAD, do_masound, -1, ADMLVL_NONE, 0},
    {"mheal", "mhea", POS_SITTING, do_mheal, -1, ADMLVL_NONE, 0},
    {"mkill", "mkill", POS_STANDING, do_mkill, -1, ADMLVL_NONE, 0},
    {"mjunk", "mjunk", POS_SITTING, do_mjunk, -1, ADMLVL_NONE, 0},
    {"mdamage", "mdamage", POS_DEAD, do_mdamage, -1, ADMLVL_NONE, 0},
    {"mdoor", "mdoor", POS_DEAD, do_mdoor, -1, ADMLVL_NONE, 0},
    {"mecho", "mecho", POS_DEAD, do_mecho, -1, ADMLVL_NONE, 0},
    {"mechoaround", "mechoaround", POS_DEAD, do_mechoaround, -1, ADMLVL_NONE,
     0},
    {"msend", "msend", POS_DEAD, do_msend, -1, ADMLVL_NONE, 0},
    {"mload", "mload", POS_DEAD, do_mload, -1, ADMLVL_NONE, 0},
    {"mpurge", "mpurge", POS_DEAD, do_mpurge, -1, ADMLVL_NONE, 0},
    {"mgoto", "mgoto", POS_DEAD, do_mgoto, -1, ADMLVL_NONE, 0},
    {"mat", "mat", POS_DEAD, do_mat, -1, ADMLVL_NONE, 0},
    {"mteleport", "mteleport", POS_DEAD, do_mteleport, -1, ADMLVL_NONE, 0},
    {"mforce", "mforce", POS_DEAD, do_mforce, -1, ADMLVL_NONE, 0},
    {"mremember", "mremember", POS_DEAD, do_mremember, -1, ADMLVL_NONE, 0},
    {"mforget", "mforget", POS_DEAD, do_mforget, -1, ADMLVL_NONE, 0},
    {"mtransform", "mtransform", POS_DEAD, do_mtransform, -1, ADMLVL_NONE, 0},
    {"mzoneecho", "mzoneecho", POS_DEAD, do_mzoneecho, -1, ADMLVL_NONE, 0},
    {"vdelete", "vdelete", POS_DEAD, do_vdelete, 0, ADMLVL_BUILDER, 0},
    {"mfollow", "mfollow", POS_DEAD, do_mfollow, -1, ADMLVL_NONE, 0},

    {"\n", "zzzzzzz", 0, 0, 0, ADMLVL_NONE, 0}}; /* this must be last */

int command_pass(char *cmd, struct char_data *ch) {

  if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
    if (strcasecmp(cmd, "liquefy") && strcasecmp(cmd, "ingest") &&
        strcasecmp(cmd, "look") && strcasecmp(cmd, "score") &&
        strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") &&
        strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") &&
        strcasecmp(cmd, "status")) {
      send_to_char(
          ch,
          "You are not capable of performing that action while liquefied!\r\n");
      return (FALSE);
    }
  } else if (IS_AFFECTED(ch, AFF_PARALYZE)) {
    if (strcasecmp(cmd, "look") && strcasecmp(cmd, "score") &&
        strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") &&
        strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") &&
        strcasecmp(cmd, "status")) {
      send_to_char(
          ch,
          "You are not capable of performing that action while petrified!\r\n");
      return (FALSE);
    }
  } else if (IS_AFFECTED(ch, AFF_FROZEN)) {
    if (strcasecmp(cmd, "look") && strcasecmp(cmd, "score") &&
        strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") &&
        strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") &&
        strcasecmp(cmd, "status")) {
      send_to_char(ch, "You are not capable of performing that action while a "
                       "frozen block of ice!\r\n");
      return (FALSE);
    }
  } else if (IS_AFFECTED(ch, AFF_PARA) && GET_INT(ch) < rand_number(1, 60)) {
    if (strcasecmp(cmd, "look") && strcasecmp(cmd, "score") &&
        strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") &&
        strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") &&
        strcasecmp(cmd, "status")) {
      act("@yYou fail to overcome your paralysis!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@Y$n @ystruggles with $s paralysis!@n", TRUE, ch, 0, 0, TO_ROOM);
      return (FALSE);
    }
  }

  return (TRUE);
}

int special(struct char_data *ch, int cmd, char *arg) {
  struct char_data *k;

  struct room_data *room = char_room_get(ch);

  /* special in room? */
  if (room_func_get(room))
    if (room_func_get(room)(ch, room, cmd, arg))
      return (1);

  /* special in equipment list? */
  {
    bool found = false;
    char_equipment_iterate(ch, [&](auto j, auto eq) {
      if (GET_OBJ_SPEC(eq))
        if (GET_OBJ_SPEC(eq)(ch, eq, cmd, arg)) {
          found = true;
          return false;
        }
      return true;
    });
    if (found)
      return (1);
  }

  /* special in inventory? */
  {
    bool found = false;
    char_inventory_iterate(ch, [&](auto i) {
      if (GET_OBJ_SPEC(i) != NULL)
        if (GET_OBJ_SPEC(i)(ch, i, cmd, arg)) {
          found = true;
          return false;
        }
      return true;
    });
    if (found) return (1);
  }

  /* special in mobile present? */
  {
    bool found = false;
    room_people_iterate(char_room_get(ch), [&](auto k) {
      if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
        if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k)(ch, k, cmd, arg)) {
          found = true;
          return false;
        }
      return true;
    });
    if (found)
      return (1);
  }

  /* special in object present? */
  {
    int spec_result = 0;
    room_contents_iterate(char_room_get(ch), [&](auto i) {
      if (GET_OBJ_SPEC(i) != NULL)
        if (GET_OBJ_SPEC(i)(ch, i, cmd, arg)) {
          spec_result = 1;
          return false;
        }
      return true;
    });
    if (spec_result) return (1);
  }

  return (0);
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument) {
  int cmd, length;
  int skip_ld = 0;
  char *line;
  char arg[MAX_INPUT_LENGTH];

  switch (GET_POS(ch)) {
  case POS_DEAD:
  case POS_INCAP:
  case POS_MORTALLYW:
  case POS_STUNNED:
    char_position_set(ch, POS_SITTING);
    break;
  }

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  if (!strcasecmp(arg, "-")) {
    return;
  }
  /* Since all command triggers check for valid_dg_target before acting, the
   * levelcheck here has been removed.
   */
  /* otherwise, find the command */
  {
    int cont;                               /* continue the command checks */
    cont = command_wtrigger(ch, arg, line); /* any world triggers ? */
    if (!cont)
      cont = command_mtrigger(ch, arg, line); /* any mobile triggers ? */
    if (!cont)
      cont = command_otrigger(ch, arg, line); /* any object triggers ? */
    if (cont)
      return; /* yes, command trigger took over */
  }

  /* Try Lua commands before the hardcoded table. */
  if (char_command_try(ch, arg, line))
    return;

  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n';
       cmd++) {
    if (!strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level &&
          GET_ADMLEVEL(ch) >= complete_cmd_info[cmd].minimum_admlevel)
        break;
  }

  char blah[MAX_INPUT_LENGTH];

  sprintf(blah, "%s", complete_cmd_info[cmd].command);

  if (*complete_cmd_info[cmd].command == '\n') {
    send_to_char(ch, "Huh!?!\r\n");
    return;
  }

  else if (!command_pass(blah, ch) && GET_ADMLEVEL(ch) < 1)
    send_to_char(ch, "It's unfortunate...\r\n");
  else if (check_disabled(&complete_cmd_info[cmd])) /* is it disabled? */
    send_to_char(ch, "This command has been temporarily disabled.\r\n");
  else if (!IS_NPC(ch) && char_condition_has_tag(ch, "goop") &&
           GET_ADMLEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You only have your internal thoughts until your body has "
                     "finished regenerating!\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) &&
           GET_ADMLEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (!IS_NPC(ch) && char_condition_has(ch, "spiral"))
    send_to_char(ch, "You are occupied with your Spiral Comet attack!\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (IS_NPC(ch) &&
           complete_cmd_info[cmd].minimum_admlevel >= ADMLVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position &&
           GET_POS(ch) != POS_FIGHTING) {
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch,
                   "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch,
                   "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      break;
    }
  } else if (no_specials || !special(ch, cmd, line)) {
    if (!skip_ld) {
      ((*complete_cmd_info[cmd].command_pointer)(
          ch, line, cmd, complete_cmd_info[cmd].subcmd));
    }
  }
}
