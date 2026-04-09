#include "dbat/game/command.h"



const struct command_info cmd_info[] = {
  { "RESERVED", "", 0, 0, 0, ADMLVL_NONE	, 0 },     /* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , "n"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NORTH },
  { "east"     , "e"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_EAST },
  { "south"    , "s"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SOUTH },
  { "west"     , "w"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_WEST },
  { "up"       , "u"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_UP },
  { "down"     , "d"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_DOWN },
  { "northwest", "northw"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "nw"       , "nw"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "northeast", "northe"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "ne"       , "ne"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "southeast", "southe"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "se"       , "se"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "southwest", "southw"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },
  { "sw"       , "sw"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },
  { "i"        , "i"            , POS_DEAD    , do_inventory, 0, ADMLVL_NONE    , 0 },
  { "inside"   , "in"      , POS_RESTING, do_move     , 0, ADMLVL_NONE , SCMD_IN },
  { "outside"  , "out"     , POS_RESTING, do_move     , 0, ADMLVL_NONE , SCMD_OUT },

  /* now, the main list */
  { "absorb"   , "absor"        , POS_FIGHTING, do_absorb   , 0, ADMLVL_NONE    , 0 },
  { "at"       , "at"		, POS_DEAD    , do_at       , 0, ADMLVL_BUILDER	, 0 },
  { "adrenaline"  , "adrenalin" , POS_DEAD    , do_adrenaline, 0, ADMLVL_NONE    , 0 },
  { "advance"  , "adv"		, POS_DEAD    , do_advance  , 0, ADMLVL_IMPL	, 0 },
  { "aedit"    , "aed"	 	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_AEDIT },
  { "alias"    , "ali"		, POS_DEAD    , do_alias    , 0, ADMLVL_NONE	, 0 },
  { "afk"      , "afk"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AFK },
  { "aid"      , "aid"          , POS_STANDING, do_aid      , 0, ADMLVL_NONE    , 0 },
  { "amnesiac" , "amnesia"      , POS_STANDING, do_amnisiac , 0, ADMLVL_NONE    , 0 },
  { "appraise" , "apprais"      , POS_STANDING, do_appraise , 0, ADMLVL_NONE    , 0 },
  { "approve"  , "approve"      , POS_STANDING, do_approve  , 0, ADMLVL_IMMORT  , 0 },
  { "arena"    , "aren"         , POS_RESTING , do_arena    , 0, ADMLVL_NONE    , 0 },
  { "ashcloud" , "ashclou"      , POS_RESTING , do_ashcloud , 0, ADMLVL_NONE    , 0 },
  { "assedit"  , "assed"	, POS_STANDING, do_assedit  , 0, ADMLVL_GOD	, 0},
  { "assist"   , "assis"        , POS_STANDING, do_assist   , 0, ADMLVL_NONE    , 0 },
  { "astat"    , "ast"		, POS_DEAD    , do_astat    , 0, ADMLVL_GOD	, 0 },
  { "ask"      , "ask"		, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_ASK },
  { "attack"   , "attack"       , POS_FIGHTING, do_attack   , 0, 0, 0 },
  { "auction"  , "auctio"       , POS_RESTING , do_not_here , 0, 0, 0 },
  { "augment"  , "augmen"       , POS_SITTING , do_not_here , 1, ADMLVL_NONE    , 0 },
  { "aura"     , "aura"         , POS_RESTING , do_aura , 0,  ADMLVL_NONE     , 0 },
  { "autoexit" , "autoex"	, POS_DEAD    , do_autoexit , 0, ADMLVL_NONE	, 0 },
  { "autogold" , "autogo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOGOLD },
  { "autoloot" , "autolo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOLOOT },
  { "autosplit", "autosp"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_AUTOSPLIT },

  { "bakuhatsuha", "baku"       , POS_FIGHTING, do_baku   , 0, 0, 0 },
  { "ban"      , "ban"		, POS_DEAD    , do_ban      , 0, ADMLVL_VICE	, 0 },
  { "balance"  , "bal"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "balefire"  , "balef"       , POS_FIGHTING, do_balefire  , 0, ADMLVL_NONE    , 0 },
  { "barrage"  , "barrage"      , POS_FIGHTING, do_pbarrage , 0, ADMLVL_NONE     , 0 },
  { "barrier"  , "barri"        , POS_FIGHTING, do_barrier  , 0, ADMLVL_NONE    , 0 },
  { "bash"     , "bas"          , POS_FIGHTING, do_bash     , 0, ADMLVL_NONE    , 0 },
  { "beam"     , "bea"          , POS_FIGHTING, do_beam     , 0, ADMLVL_NONE    , 0 },
  { "bexchange" , "bexchan"       , POS_RESTING , do_rbanktrans  , 0, ADMLVL_NONE    , 0 },
  { "bid"      , "bi"           , POS_RESTING , do_bid      , 0, 0, 0 },
  { "bigbang"  , "bigban"       , POS_FIGHTING, do_bigbang  , 0, 0, 0 },
  { "bite"     , "bit"          , POS_FIGHTING, do_bite     , 0, 0, 0 },
  { "blessedhammer", "bham"    , POS_FIGHTING, do_blessedhammer, 0, ADMLVL_NONE    , 0 },
  { "block"    , "block"        , POS_FIGHTING, do_block    , 0, 0, 0 },
  { "book"     , "boo"          , POS_SLEEPING, do_gen_ps   , 0, ADMLVL_IMMORT  , SCMD_INFO },
  { "break"    , "break"	, POS_STANDING, do_break    , 0, ADMLVL_IMMORT	, 0 },
  { "brief"    , "br"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_BRIEF },
  { "build"    , "bui"          , POS_SITTING , do_assemble , 0, ADMLVL_NONE    , SCMD_BREW },
  { "buildwalk", "buildwalk"    , POS_STANDING, do_gen_tog, 0, ADMLVL_IMMORT    , SCMD_BUILDWALK },
  { "buy"      , "bu"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "bug"      , "bug"		, POS_DEAD    , do_gen_write, 0, ADMLVL_NONE	, SCMD_BUG },

  { "cancel"   , "cance"        , POS_RESTING , do_not_here , 0, 0, 0 },
  { "candy"    , "cand"         , POS_FIGHTING, do_candy    , 0, 0, 0 },
  { "carry"    , "carr"         , POS_STANDING, do_carry    , 0, 0, 0 },
  { "carve"    , "carv"         , POS_SLEEPING, do_gen_tog   , 0, 0, SCMD_CARVE },
  { "cedit"    , "cedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMPL	, SCMD_OASIS_CEDIT },
  { "channel"  , "channe"       , POS_FIGHTING, do_channel  , 0, 0, 0 },
  { "charge"   , "char"         , POS_FIGHTING, do_charge   , 0, 0, 0 },
  { "check"    , "ch"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "checkload", "checkl"	, POS_DEAD    , do_checkloadstatus, 0, ADMLVL_GOD, 0 },
  { "chown"    , "cho"		, POS_DEAD    , do_chown    , 1, ADMLVL_IMPL	, 0 },
  { "clan"     , "cla"          , POS_DEAD    , do_clan     , 0, ADMLVL_NONE     , 0 },
  { "clear"    , "cle"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "close"    , "cl"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_CLOSE },
  { "closeeyes", "closeey"      , POS_RESTING , do_eyec     , 0, ADMLVL_NONE    , 0 },
  { "cls"      , "cls"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "clsolc"   , "clsolc"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_BUILDER	, SCMD_CLS },
  { "consider" , "con"		, POS_RESTING , do_consider , 0, ADMLVL_NONE	, 0 },
  { "color"    , "col"		, POS_DEAD    , do_color    , 0, ADMLVL_NONE	, 0 },
  { "combine"  , "comb"         , POS_RESTING , do_combine  , 0, ADMLVL_NONE    , 0 },
  { "compare"  , "comp"		, POS_RESTING , do_compare  , 0, ADMLVL_NONE	, 0 },
  { "commands" , "com"		, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_COMMANDS },
  { "commune"  , "comm"         , POS_DEAD    , do_commune  , 0, ADMLVL_NONE    , 0 },
  { "compact"  , "compact"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_COMPACT },
  { "cook"     , "coo"          , POS_RESTING , do_cook     , 0, ADMLVL_NONE    , 0 },
  { "copyover" , "copyover"	, POS_DEAD    , do_copyover , 0, ADMLVL_GOD	, 0 },
  { "create"   , "crea"         , POS_STANDING, do_form     , 0, ADMLVL_NONE    , 0 },
  { "credits"  , "cred"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CREDITS },
  { "crusher"  , "crushe"       , POS_FIGHTING, do_crusher  , 0, 0, 0 },

  { "date"     , "da"		, POS_DEAD    , do_date     , 0, ADMLVL_IMMORT	, SCMD_DATE },
  { "darkness" , "darknes"      , POS_FIGHTING, do_ddslash  , 0, ADMLVL_NONE    , 0 },
  { "dc"       , "dc"		, POS_DEAD    , do_dc       , 0, ADMLVL_GOD	, 0 },
  { "deathball", "deathbal"     , POS_FIGHTING, do_deathball, 0, ADMLVL_NONE    , 0 },
  { "deathbeam", "deathbea"     , POS_FIGHTING, do_deathbeam, 0, ADMLVL_NONE    , 0 },
  { "decapitate", "decapit"     , POS_STANDING, do_spoil    , 0, ADMLVL_NONE    , 0 },
  { "defend"   , "defen"        , POS_STANDING, do_defend   , 0, ADMLVL_NONE    , 0 },
  { "deploy"   , "deplo"        , POS_STANDING, do_deploy   , 0, ADMLVL_NONE    , 0 },
  { "dualbeam" , "dualbea"      , POS_FIGHTING, do_dualbeam , 0, ADMLVL_NONE    , 0 },
  { "deposit"  , "depo"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "diagnose" , "diagnos"      , POS_RESTING , do_diagnose , 0, ADMLVL_NONE    , 0 },
  { "dimizu"   , "dimizu"       , POS_STANDING, do_dimizu   , 0, 0   , 0 },
  { "disable"  , "disa"		, POS_DEAD    , do_disable  , 0, ADMLVL_VICE	, 0 },
  { "disguise" , "disguis"      , POS_DEAD    , do_disguise , 0, 0, 0 },
  { "dig"      , "dig"		, POS_DEAD    , do_bury     , 0, ADMLVL_NONE	, 0 },
  { "display"  , "disp"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "dodonpa"  , "dodon"        , POS_FIGHTING, do_dodonpa  , 0, ADMLVL_NONE    , 0 },
  { "donate"   , "don"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DONATE },
  { "drag"     , "dra"          , POS_STANDING, do_drag     , 0, ADMLVL_NONE    , 0 },
  { "draw"     , "dra"          , POS_SITTING , do_draw     , 0, ADMLVL_NONE    , 0 },
  { "drink"    , "dri"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_DRINK },
  { "drop"     , "dro"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DROP },
  { "dub"      , "du"           , POS_STANDING, do_intro  , 0, ADMLVL_NONE    , 0 },

  { "eat"      , "ea"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_EAT },
  { "eavesdrop", "eaves"       , POS_RESTING , do_eavesdrop, 0, ADMLVL_NONE    , 0 },
  { "echo"     , "ec"		, POS_SLEEPING, do_echo     , 0, ADMLVL_IMMORT	, SCMD_ECHO },
  { "elbow"    , "elb"          , POS_FIGHTING, do_elbow    , 0, ADMLVL_NONE    , 0 },
  { "emote"    , "em"		, POS_RESTING , do_echo     , 1, ADMLVL_NONE	, SCMD_EMOTE },
  { "energize" , "energiz"      , POS_RESTING , do_energize , 1, ADMLVL_NONE    , 0 },
  { ":"        , ":"		, POS_RESTING, do_echo      , 1, ADMLVL_NONE	, SCMD_EMOTE },
  { "ensnare"  , "ensnar"       , POS_FIGHTING, do_ensnare  , 0, ADMLVL_NONE    , 0 },
  { "enter"    , "ent"		, POS_STANDING, do_enter    , 0, ADMLVL_NONE	, 0 },
  { "equipment", "eq"		, POS_SLEEPING, do_equipment, 0, ADMLVL_NONE	, 0 },
  { "eraser"   , "eras"         , POS_FIGHTING, do_eraser   , 0, ADMLVL_NONE    , 0 },
  { "escape"   , "esca"         , POS_RESTING,  do_escape   , 0, ADMLVL_NONE    , 0 },
  { "evolve"   , "evolv"        , POS_RESTING, do_evolve    , 0, ADMLVL_NONE    , 0 },
  { "exchange" , "exchan"       , POS_RESTING , do_rptrans  , 0, ADMLVL_NONE    , 0 },
  { "exits"    , "ex"		, POS_RESTING , do_exits    , 0, ADMLVL_NONE	, 0 },
  { "examine"  , "exa"		, POS_SITTING , do_examine  , 0, ADMLVL_NONE	, 0 },
  { "extract"  , "extrac"       , POS_STANDING, do_extract  , 0, ADMLVL_NONE    , 0 },

  { "feed"     , "fee"          , POS_STANDING, do_feed     , 0, ADMLVL_NONE    , 0 },
  { "fill"     , "fil"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_FILL },
  { "file"     , "fi"		, POS_SLEEPING, do_file     , 0, ADMLVL_IMMORT	, 0 },
  { "finalflash" , "finalflash" , POS_FIGHTING, do_final    , 0, ADMLVL_NONE    , 0 },
  { "finddoor" , "findd"	, POS_SLEEPING, do_finddoor , 0, ADMLVL_IMMORT, 0 },
  { "findkey"  , "findk"	, POS_SLEEPING, do_findkey  , 0, ADMLVL_IMMORT, 0 },
  { "finger"   , "finge"        , POS_SLEEPING, do_finger   , 0, ADMLVL_NONE    , 0 },
  { "fireshield"  , "firesh"    , POS_STANDING, do_fireshield   , 0, ADMLVL_NONE    , 0 },
  { "fish"     , "fis"          , POS_STANDING, do_fish     , 0, ADMLVL_NONE  , 0 },
  { "fix"      , "fix"		, POS_STANDING, do_fix      , 0, ADMLVL_NONE	, 0 },
  { "flee"     , "fl"		, POS_FIGHTING, do_flee     , 1, ADMLVL_NONE	, 0 },
  { "fly"      , "fly"          , POS_RESTING, do_fly      , 0, ADMLVL_NONE    , 0 },
  { "focus"    , "foc"          , POS_STANDING, do_focus    , 0, ADMLVL_NONE    , 0 },
  { "follow"   , "fol"		, POS_RESTING , do_follow   , 0, ADMLVL_NONE	, 0 },
  { "force"    , "force"        , POS_SLEEPING, do_force    , 0, ADMLVL_IMMORT  , 0 },
  { "forgery"  , "forg"         , POS_RESTING, do_forgery   , 0, ADMLVL_NONE    , 0 },
  { "forget"   , "forg"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "freeze"   , "freeze"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMMORT	, SCMD_FREEZE },
  { "fury"     , "fury"         , POS_FIGHTING, do_fury     , 0, ADMLVL_NONE    , 0 },
  { "future"   , "futu"         , POS_STANDING, do_future   , 0, ADMLVL_NONE    , 0 },

  { "gain"     , "ga"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "galikgun" , "galik"        , POS_FIGHTING, do_galikgun , 0, ADMLVL_NONE    , 0 },
  { "game"     , "gam"          , POS_RESTING , do_show     , 0, ADMLVL_IMMORT  , 0 },
  { "garden"   , "garde"        , POS_STANDING, do_garden   , 0, ADMLVL_NONE    , 0 },
  { "genkidama", "genkidam"     , POS_FIGHTING, do_genki    , 0, ADMLVL_NONE    , 0 },
  { "genocide" , "genocid"      , POS_FIGHTING, do_geno     , 0, ADMLVL_NONE    , 0 },
  { "get"      , "get"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "gecho"    , "gecho"	, POS_DEAD    , do_gecho    , 0, ADMLVL_BUILDER	, 0 },
  { "gedit"    , "gedit"      	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_GEDIT },
  { "gemote"   , "gem"	 	, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GEMOTE },
  { "generator", "genr"         , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "glist"    , "glist"	, POS_SLEEPING, do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_GLIST },
  { "give"     , "giv"		, POS_RESTING , do_give     , 0, ADMLVL_NONE	, 0 },
  { "goto"     , "go"		, POS_SLEEPING, do_goto     , 0, ADMLVL_IMMORT	, 0 },
  { "gold"     , "gol"		, POS_RESTING , do_gold     , 0, ADMLVL_NONE	, 0 },
  { "group"    , "gro"		, POS_RESTING , do_group    , 1, ADMLVL_NONE	, 0 },
  { "grab"     , "grab"		, POS_RESTING , do_grab     , 0, ADMLVL_NONE	, 0 },
  { "grand"    , "gran"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "grapple"  , "grapp"        , POS_FIGHTING, do_grapple  , 0, ADMLVL_NONE    , 0 },
  { "grats"    , "grat"		, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GRATZ },
  { "gravity"  , "grav"         , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "gsay"     , "gsay"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },
  { "gtell"    , "gt"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },

  { "hand"     , "han"          , POS_SITTING , do_hand     , 0, ADMLVL_NONE    , 0 },
  { "handout"  , "hand"         , POS_STANDING, do_handout  , 0, ADMLVL_GOD     , 0 },
  { "hasshuken", "hasshuke"     , POS_STANDING, do_hass     , 0, ADMLVL_NONE    , 0 },
  { "hayasa"   , "hayas"       , POS_STANDING, do_hayasa   , 0, ADMLVL_NONE    , 0 },
  { "headbutt" , "headbut"      , POS_FIGHTING, do_head     , 0, ADMLVL_NONE    , 0 },
  { "heal"     , "hea"          , POS_STANDING, do_heal     , 0, ADMLVL_NONE    , 0 },
  { "health"   , "hea"          , POS_DEAD,     do_gen_tog  , 0, ADMLVL_NONE    , SCMD_GHEALTH },
  { "healingglow" , "healing"   , POS_STANDING, do_healglow     , 0, ADMLVL_NONE    , 0 },
  { "heeldrop" , "heeldr"       , POS_FIGHTING, do_heeldrop , 0, ADMLVL_NONE    , 0 },
  { "hellflash", "hellflas"     , POS_FIGHTING, do_hellflash, 0, ADMLVL_NONE     , 0 },
  { "hellspear", "hellspea"     , POS_FIGHTING, do_hellspear, 0, ADMLVL_NONE     , 0 },
  { "help"     , "h"            , POS_DEAD    , do_help     , 0, ADMLVL_NONE    , 0 },
  { "hedit"    , "hedit"        , POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT  , SCMD_OASIS_HEDIT },
  { "hindex"   , "hind"         , POS_DEAD    , do_hindex   , 0, ADMLVL_NONE    , 0 },
  { "helpcheck", "helpch"       , POS_DEAD    , do_helpcheck, 0, ADMLVL_NONE    , 0 },
  { "handbook" , "handb"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_IMMORT	, SCMD_HANDBOOK },
  { "hide"     , "hide"		, POS_RESTING , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_HIDE },
  { "hints"    , "hints"        , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_HINTS },
  { "history"  , "hist"         , POS_DEAD    , do_history  , 0, ADMLVL_NONE    , 0 },
  { "hold"     , "hold"		, POS_RESTING , do_grab     , 1, ADMLVL_NONE	, 0 },
  { "holylight", "holy"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_HOLYLIGHT },
  { "honoo"    , "hono"         , POS_FIGHTING, do_honoo    , 0, ADMLVL_NONE    , 0 },
  { "house"    , "house"	, POS_RESTING , do_house    , 0, ADMLVL_NONE	, 0 },
  { "hsedit"   , "hsedit"       , POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER , SCMD_OASIS_HSEDIT },
  { "hspiral"  , "hspira"       , POS_FIGHTING, do_hspiral  , 0, ADMLVL_NONE    , 0 },
  { "htank"    , "htan"         , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "hydromancy", "hydrom"      , POS_STANDING, do_hydromancy , 0, ADMLVL_NONE    , 0 },
  { "hyoga"    , "hyoga"        , POS_STANDING, do_obstruct , 0, ADMLVL_NONE    , 0 },

  { "ihealth"  , "ihea"         , POS_DEAD,     do_gen_tog  , 0, ADMLVL_NONE    , SCMD_IHEALTH },
  { "info"     , "info"         , POS_DEAD    , do_ginfo    , 0, ADMLVL_IMMORT  , 0 },
  { "infuse"   , "infus"        , POS_STANDING, do_infuse   , 0, ADMLVL_NONE    , 0 },
  { "ingest"   , "inges"        , POS_STANDING, do_ingest   , 0, ADMLVL_NONE    , 0 },
  { "imotd"    , "imotd"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_IMMORT	, SCMD_IMOTD },
  { "immlist"  , "imm"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WIZLIST },
  { "implant"  , "implan"       , POS_RESTING , do_implant  , 0, ADMLVL_NONE    , 0 },
  { "instant"  , "insta"        , POS_STANDING, do_instant  , 0, ADMLVL_NONE    , 0 },
  { "instill"  , "instil"       , POS_STANDING, do_instill  , 0, ADMLVL_NONE    , 0 },
  { "instruct" , "instruc"      , POS_STANDING, do_gen_tog   , 0, 0, SCMD_INSTRUCT },
  /*{ "insult"   , "insult"	, POS_RESTING , do_insult   , 0, ADMLVL_NONE	, 0 },*/
  { "inventory", "inv"		, POS_DEAD    , do_inventory, 0, ADMLVL_NONE	, 0 },
  { "interest" , "inter"        , POS_DEAD    , do_interest , 0, ADMLVL_IMPL    , 0 },
  { "iedit"    , "ie"   	, POS_DEAD    , do_iedit    , 0, ADMLVL_IMPL	, 0 },
  { "invis"    , "invi"		, POS_DEAD    , do_invis    , 0, ADMLVL_IMMORT	, 0 },
  { "iwarp"    , "iwarp"        , POS_RESTING , do_warp     , 0, ADMLVL_NONE    , 0 },

  { "junk"     , "junk"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_JUNK },

  { "kaioken"  , "kaioken"      , POS_STANDING, do_kaioken  , 0, ADMLVL_NONE    , 0 },
  { "kakusanha", "kakusan"      , POS_FIGHTING, do_kakusanha, 0, ADMLVL_NONE    , 0 },
  { "kamehameha", "kame"        , POS_FIGHTING, do_kamehameha, 0, ADMLVL_NONE    , 0 },
  { "kanso"    , "kans"         , POS_FIGHTING, do_kanso    , 0, ADMLVL_NONE    , 0 },
  { "kiball"   , "kibal"        , POS_FIGHTING, do_kiball   , 0, ADMLVL_NONE    , 0 },
  { "kiblast"  , "kiblas"       , POS_FIGHTING, do_kiblast  , 0, ADMLVL_NONE    , 0 },
  { "kienzan"  , "kienza"       , POS_FIGHTING, do_kienzan  , 0, ADMLVL_NONE    , 0 },
  { "kill"     , "kil"		, POS_FIGHTING, do_kill     , 0, ADMLVL_IMMORT	, 0 },
  { "kick"     , "kic"          , POS_FIGHTING, do_kick     , 0, ADMLVL_NONE     , 0 },
  { "knee"     , "kne"          , POS_FIGHTING, do_knee     , 0, ADMLVL_NONE     , 0 },
  { "koteiru"  , "koteiru"      , POS_FIGHTING, do_koteiru  , 0, ADMLVL_NONE    , 0 },
  { "kousengan", "kousengan"    , POS_FIGHTING, do_kousengan, 0, ADMLVL_NONE    , 0 },
  { "kuraiiro" , "kuraiir"      , POS_FIGHTING, do_kura     , 0, ADMLVL_NONE     , 0 },
  { "kyodaika" , "kyodaik"      , POS_STANDING, do_kyodaika , 0, ADMLVL_NONE    , 0 },

  { "look"     , "lo"		, POS_RESTING , do_look     , 0, ADMLVL_NONE	, SCMD_LOOK },
  { "lag"      , "la"           , POS_RESTING , do_lag      , 0, 5   , 0 },
  { "land"     , "lan"          , POS_RESTING , do_land     , 0, ADMLVL_NONE    , 0 },
  { "languages", "lang"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "last"     , "last"		, POS_DEAD    , do_last     , 0, ADMLVL_GOD	, 0 },
  { "learn"    , "lear"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "leave"    , "lea"		, POS_STANDING, do_leave    , 0, ADMLVL_NONE	, 0 },
  { "levels"   , "lev"		, POS_DEAD    , do_levels   , 0, ADMLVL_NONE	, 0 },
  { "light"    , "ligh"         , POS_STANDING, do_lightgrenade, 0, ADMLVL_NONE , 0 },
  { "list"     , "lis"          , POS_STANDING, do_not_here , 0, ADMLVL_NONE    , 0 },
  { "life"     , "lif"		, POS_SLEEPING, do_lifeforce, 0, ADMLVL_NONE	, 0 },
  { "links"    , "lin"		, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_LINKS },
  { "liquefy"  , "liquef"       , POS_SLEEPING, do_liquefy  , 0, ADMLVL_NONE    , 0 },
  { "lkeep"    , "lkee"         , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_LKEEP },
  { "lock"     , "loc"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_LOCK },
  { "lockout"  , "lock"         , POS_STANDING, do_hell     , 0, ADMLVL_IMMORT  , 0 },
  { "load"     , "load"		, POS_DEAD    , do_load     , 0, ADMLVL_IMMORT	, 0 },

  { "majinize" , "majini"       , POS_STANDING, do_majinize , 0, ADMLVL_NONE    , 0 },
  { "malice"   , "malic"        , POS_FIGHTING, do_malice   , 0, ADMLVL_NONE    , 0 },
  { "masenko"  , "masenk"       , POS_FIGHTING, do_masenko  , 0, ADMLVL_NONE    , 0 },
  { "motd"     , "motd"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_MOTD },
  { "mail"     , "mail"		, POS_STANDING, do_not_here , 2, ADMLVL_NONE	, 0 },
  { "map"      , "map"          , POS_STANDING, do_map      , 0, ADMLVL_NONE    , 0 },
  /*{ "mcopy"    , "mcopy"	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, SCMD_MEDIT },*/
  { "medit"    , "medit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_MEDIT },
  { "meditate" , "medita"       , POS_SITTING, do_meditate, 0, ADMLVL_NONE    , 0 },
  { "metamorph", "metamorp"     , POS_STANDING, do_metamorph, 0, ADMLVL_NONE    , 0 },
  { "mimic"    , "mimi"         , POS_STANDING, do_mimic    , 0, ADMLVL_NONE    , 0 },
  { "mlist"    , "mlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_MLIST },
  { "moondust" , "moondus"      , POS_STANDING, do_moondust , 0, ADMLVL_NONE    , 0 },
  { "multiform", "multifor"     , POS_STANDING, do_multiform , 0, ADMLVL_NONE    , 0 },
  { "mute"     , "mute"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMMORT	, SCMD_SQUELCH },
  { "music"    , "musi"         , POS_RESTING , do_gen_comm , 1, ADMLVL_NONE  , SCMD_HOLLER },

  { "newbie"   , "newbie"       , POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE    , SCMD_AUCTION },
  { "news"     , "news"		, POS_SLEEPING, do_news     , 0, ADMLVL_NONE	, 0 },
  { "newsedit" , "newsedi"      , POS_SLEEPING, do_newsedit , 0, ADMLVL_IMMORT  , 0 },
  { "nickname" , "nicknam"      , POS_RESTING , do_nickname , 0, ADMLVL_NONE    , 0 },
  { "nocompress","nocompress"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOCOMPRESS },
  { "noeq"     , "noeq"         , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOEQSEE },
  { "nolin"    , "nolin"        , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NODEC },
  { "nomusic"  , "nomusi"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOMUSIC },
  { "noooc"    , "noooc"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGOSSIP },
  { "nogive"   , "nogiv"        , POS_DEAD    , do_gen_tog   , 0, 0, SCMD_NOGIVE },
  { "nograts"  , "nograts"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGRATZ },
  { "nogrow"   , "nogro"        , POS_DEAD    , do_nogrow   , 0, ADMLVL_NONE    , 0 },
  { "nohassle" , "nohassle"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_NOHASSLE },
  { "nomail"   , "nomail"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NMWARN },
  { "nonewbie" , "nonewbie"     , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOAUCTION },
  { "noparry"  , "noparr"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOPARRY },
  { "norepeat" , "norepeat"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOREPEAT },
  { "noshout"  , "noshout"	, POS_SLEEPING, do_gen_tog  , 1, ADMLVL_NONE	, SCMD_DEAF },
  { "nosummon" , "nosummon"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_NOSUMMON },
  { "notell"   , "notell"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_NOTELL },
  { "notitle"  , "notitle"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_NOTITLE },
  { "nova"     , "nov"          , POS_STANDING, do_nova     , 0, ADMLVL_NONE    , 0 },
  { "nowiz"    , "nowiz"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_NOWIZ },

  /*{ "ocopy"    , "ocopy"	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, SCMD_OEDIT },*/
  { "ooc"      , "ooc"          , POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE     , SCMD_GOSSIP },
  { "offer"    , "off"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "open"     , "ope"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_OPEN },
  { "olc"      , "olc"		, POS_DEAD    , do_show_save_list, 0, ADMLVL_IMMORT, 0 },
  { "olist"    , "olist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_OLIST },
  { "oedit"    , "oedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_OEDIT },
  { "osay"     , "osay"         , POS_RESTING , do_osay     , 0, ADMLVL_NONE     , 0 },

  { "pack"     , "pac"          , POS_STANDING, do_pack     , 0, 0 , 0 },
  { "page"     , "pag"		, POS_DEAD    , do_page     , 0, ADMLVL_BUILDER	, 0 },
  { "paralyze" , "paralyz"      , POS_FIGHTING, do_paralyze , 0, ADMLVL_NONE    , 0 },
  { "pagelength", "pagel"	, POS_DEAD    , do_pagelength, 0, 0, 0 },
  { "peace"    , "pea"		, POS_DEAD    , do_peace    , 0, ADMLVL_BUILDER	, 0 },
  { "perfect"  , "perfec"       , POS_DEAD    , do_perf     , 0, ADMLVL_NONE , 0 },
  { "permission", "permiss"     , POS_DEAD    , do_permission, 0, ADMLVL_IMMORT , 0 },
  { "phoenix"  , "phoeni"       , POS_FIGHTING, do_pslash   , 0, ADMLVL_NONE    , 0 },
  { "pick"     , "pi"		, POS_STANDING, do_gen_door , 1, ADMLVL_NONE	, SCMD_PICK },
  { "pickup"   , "picku"        , POS_RESTING , do_not_here , 0, 0, 0 },
  { "pilot"    , "pilot"	, POS_SITTING , do_drive    , 0, ADMLVL_NONE	, 0 },
  { "plant"    , "plan"         , POS_STANDING, do_plant    , 0, ADMLVL_NONE    , 0 },
  { "play"     , "pla"          , POS_SITTING , do_play     , 0, ADMLVL_NONE    , 0 },
  { "players"  , "play"		, POS_DEAD    , do_plist    , 0, ADMLVL_IMPL	, 0 },
  { "poofin"   , "poofi"	, POS_DEAD    , do_poofset  , 0, ADMLVL_IMMORT	, SCMD_POOFIN },
  { "poofout"  , "poofo"	, POS_DEAD    , do_poofset  , 0, ADMLVL_IMMORT	, SCMD_POOFOUT },
  { "pose"     , "pos"          , POS_STANDING, do_pose     , 0, ADMLVL_NONE    , 0 },
  { "post"     , "pos"          , POS_STANDING, do_post     , 0, ADMLVL_NONE    , 0 },
  { "potential", "poten"        , POS_STANDING, do_potential, 0, ADMLVL_NONE    , 0 },
  { "pour"     , "pour"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_POUR },
  { "powerup"  , "poweru"       , POS_FIGHTING, do_powerup  , 0, ADMLVL_NONE    , 0 },
  { "preference", "preferenc"   , POS_DEAD    , do_preference , 0, ADMLVL_NONE    , 0 },
  { "program"  , "progra"       , POS_DEAD    , do_oasis    , 0, ADMLVL_NONE  , SCMD_OASIS_REDIT },
  { "prompt"   , "pro"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "practice" , "pra"		, POS_RESTING , do_practice , 1, ADMLVL_NONE	, 0 },
  { "psychic"  , "psychi"       , POS_FIGHTING, do_psyblast , 0, ADMLVL_NONE     , 0 },
  { "punch"    , "punc"         , POS_FIGHTING, do_punch    , 0, ADMLVL_NONE     , 0 },
  { "pushup"   , "pushu"        , POS_STANDING, do_pushup   , 0, ADMLVL_NONE     , 0 },
  { "put"      , "put"          , POS_RESTING , do_put      , 0, ADMLVL_NONE    , 0 },
  { "purge"    , "purge"	, POS_DEAD    , do_purge    , 0, ADMLVL_BUILDER	, 0 },

  { "qui"      , "qui"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, 0 },
  { "quit"     , "quit"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, SCMD_QUIT },

  { "radar"    , "rada"         , POS_RESTING , do_sradar   , 0, ADMLVL_NONE    , 0 },
  { "raise"    , "rai"		, POS_DEAD    , do_raise    , 0, ADMLVL_NONE	, 0 },
  { "rbank"    , "rban"         , POS_RESTING , do_rbank    , 0, ADMLVL_IMMORT  , 0 },
  { "refuel"   , "refue"        , POS_SITTING, do_refuel   , 0, ADMLVL_NONE    , 0 },
  { "resize"   , "resiz"        , POS_STANDING, do_resize   , 0, ADMLVL_NONE    , 0 },
  { "rescue"   , "rescu"        , POS_STANDING, do_rescue   , 0, ADMLVL_NONE    , 0 },
  { "rest"     , "re"		, POS_RESTING , do_rest     , 0, ADMLVL_NONE	, 0 },
  { "restring" , "restring"       , POS_STANDING, do_restring , 0, ADMLVL_NONE    , 0 },
  { "rclone"   , "rclon"        , POS_DEAD    , do_rcopy    , 0, ADMLVL_BUILDER      , 0 },
  { "rcopy"    , "rcopy"        , POS_DEAD    , do_rcopy    , 0, ADMLVL_BUILDER      , 0 },
  { "roomdisplay" , "roomdisplay"     , POS_RESTING	, do_rdisplay	, 0, ADMLVL_NONE	, 0 },
  { "read"     , "rea"          , POS_RESTING , do_look     , 0, ADMLVL_NONE    , SCMD_READ },
  { "recall"   , "reca"         , POS_STANDING, do_recall   , 0, ADMLVL_IMMORT     , 0 },
  { "recharge" , "rechar"       , POS_STANDING, do_recharge , 0, ADMLVL_NONE    , 0 },
  { "regenerate", "regen"       , POS_RESTING , do_regenerate, 0, ADMLVL_NONE    , 0 },
  { "renzokou" , "renzo"        , POS_FIGHTING, do_renzo     , 0, ADMLVL_NONE    , 0 },
  { "repair"   , "repai"        , POS_STANDING, do_srepair   , 0, ADMLVL_NONE    , 0 },
  { "reply"    , "rep"		, POS_SLEEPING, do_reply    , 0, ADMLVL_NONE	, 0 },
  { "reward"   , "rewar"        , POS_RESTING , do_reward   , 0, ADMLVL_IMMORT  , 0 },
  { "reload"   , "reload"	, POS_DEAD    , do_reboot   , 0, 5	, 0 },
  { "receive"  , "rece"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "remove"   , "rem"		, POS_RESTING , do_remove   , 0, ADMLVL_NONE	, 0 },
  { "rent"     , "rent"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "report"   , "repor"        , POS_DEAD    , do_gen_write, 0, ADMLVL_NONE    , SCMD_IDEA },
  { "reroll"   , "rero"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMPL	, SCMD_REROLL },
  { "respond"  , "resp" 	, POS_RESTING,  do_respond  , 1, ADMLVL_NONE	, 0 },
  { "restore"  , "resto"	, POS_DEAD    , do_restore  , 0, ADMLVL_GOD	, 0 },
  { "return"   , "retu"		, POS_DEAD    , do_return   , 0, ADMLVL_NONE	, 0 },
  { "redit"    , "redit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_REDIT },
  { "rip"      , "ri"           , POS_DEAD    , do_rip      , 0, ADMLVL_NONE    , 0 },
  { "rlist"    , "rlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_RLIST },
  { "rogafufuken" , "rogafu"    , POS_FIGHTING, do_rogafufuken, 0, ADMLVL_NONE    , 0 },
  { "roomflags", "roomf"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_ROOMFLAGS },
  { "roundhouse", "roundhou"    , POS_FIGHTING, do_roundhouse, 0, ADMLVL_NONE   , 0 },
  { "rpbank"   , "rpban"        , POS_SLEEPING, do_rpbank    , 0, ADMLVL_NONE   , 0 },
  { "rpp"      , "rpp"          , POS_SLEEPING, do_rpp       , 0, ADMLVL_NONE   , 0 },
  { "runic"    , "runi"         , POS_STANDING, do_runic     , 0, ADMLVL_NONE   , 0 },

  { "say"      , "say"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "'"        , "'"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "save"     , "sav"		, POS_SLEEPING, do_save     , 0, ADMLVL_NONE	, 0 },
  { "saveall"  , "saveall"	, POS_DEAD    , do_saveall  , 0, ADMLVL_BUILDER	, 0},
  { "sbc"      , "sbc"          , POS_FIGHTING, do_sbc      , 0, ADMLVL_NONE    , 0 },
  { "scan"     , "sca"          , POS_FIGHTING, do_scan     , 0, ADMLVL_NONE     , 0 },
  { "scatter"  , "scatte"       , POS_FIGHTING, do_scatter  , 0, ADMLVL_NONE    , 0 },
  { "score"    , "sc"		, POS_DEAD    , do_score    , 0, ADMLVL_NONE	, 0 },
  /*{ "scopy"    , "scopy"  	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, CON_SEDIT },*/
  { "scouter"  , "scou"         , POS_RESTING, do_scouter  , 0, ADMLVL_NONE     , 0 },
  { "scry"     , "scr"          , POS_STANDING, do_scry     , 0, ADMLVL_NONE     , 0 },
  { "seishou"  , "seisho"       , POS_FIGHTING, do_seishou  , 0, ADMLVL_NONE     , 0 },
  { "shell"    , "she"          , POS_STANDING, do_shell    , 0, ADMLVL_NONE    , 0 },
  { "shimmer"  , "shimme"       , POS_STANDING, do_shimmer  , 0, ADMLVL_NONE    , 0 },
  { "shogekiha", "shog"         , POS_STANDING, do_shogekiha, 0, ADMLVL_NONE    , 0 },
  { "shuffle"  , "shuff"        , POS_SITTING , do_shuffle  , 0, ADMLVL_NONE    , 0 },
  { "snet"     , "snet"         , POS_RESTING , do_snet     , 0, ADMLVL_NONE     , 0 },
  { "search"   , "sea"		, POS_STANDING, do_look     , 0, ADMLVL_NONE	, SCMD_SEARCH },
  { "sell"     , "sell"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "selfdestruct", "selfdest", POS_STANDING, do_selfd     , 0, ADMLVL_NONE    , 0 },
  { "sedit"    , "sedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_SEDIT },
  { "send"     , "send"		, POS_SLEEPING, do_send     , 0, ADMLVL_GOD	, 0 },
  { "sense"    , "sense"        , POS_RESTING, do_track    , 0, ADMLVL_NONE    , 0 },
  { "set"      , "set"		, POS_DEAD    , do_set      , 0, ADMLVL_IMMORT	, 0 },
  { "shout"    , "sho"		, POS_RESTING , do_gen_comm , 0, ADMLVL_NONE	, SCMD_SHOUT },
  { "show"     , "show"		, POS_DEAD    , do_showoff  , 0, ADMLVL_NONE	, 0 },
  { "shutdow"  , "shutdow"	, POS_DEAD    , do_shutdown , 0, ADMLVL_IMPL	, 0 },
  { "shutdown" , "shutdown"	, POS_DEAD    , do_shutdown , 0, ADMLVL_IMPL	, SCMD_SHUTDOWN },
  { "silk"     , "sil"          , POS_RESTING , do_silk     , 0, ADMLVL_NONE    , 0 },
  { "sip"      , "sip"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_SIP },
  { "sit"      , "sit"		, POS_RESTING , do_sit      , 0, ADMLVL_NONE	, 0 },
  { "situp"    , "situp"        , POS_STANDING, do_situp    , 0, ADMLVL_NONE     , 0 },
  { "skills"   , "skills"       , POS_SLEEPING, do_skills   , 0, ADMLVL_NONE    , 0 },
  { "skillset" , "skillset"	, POS_SLEEPING, do_skillset , 0, 5      	, 0 },
  { "slam"     , "sla"          , POS_FIGHTING, do_slam     , 0, ADMLVL_NONE    , 0 },
  { "sleep"    , "sl"		, POS_SLEEPING, do_sleep    , 0, ADMLVL_NONE	, 0 },
  { "slist"    , "slist"	, POS_SLEEPING, do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_SLIST },
  { "slowns"   , "slowns"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMPL	, SCMD_SLOWNS },
  { "smote"    , "sm"           , POS_RESTING , do_echo     , 1, ADMLVL_NONE    , SCMD_SMOTE },
  { "sneak"    , "sneak"	, POS_STANDING, do_gen_tog  , 1, ADMLVL_NONE	, SCMD_SNEAK },
  { "snoop"    , "snoop"	, POS_DEAD    , do_snoop    , 0, ADMLVL_IMMORT	, 0 },
  { "song"     , "son"          , POS_RESTING , do_song     , 0, 0  , 0 },
  { "spiral"   , "spiral"       , POS_STANDING, do_spiral   , 0, ADMLVL_NONE    , 0 },
  { "socials"  , "socials"	, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_SOCIALS },
  { "solarflare", "solarflare"  , POS_FIGHTING, do_solar    , 0, ADMLVL_NONE    , 0 },
  { "spar"     , "spa"          , POS_FIGHTING, do_spar     , 0, ADMLVL_NONE    , 0 },
  { "spit"     , "spi"          , POS_STANDING, do_spit     , 0, ADMLVL_NONE    , 0 },
  { "spiritball", "spiritball"  , POS_FIGHTING, do_spiritball, 0, ADMLVL_NONE    , 0 },
  { "spiritcontrol", "spiritcontro", POS_RESTING, do_spiritcontrol, 0, ADMLVL_NONE, 0 },
  { "split"    , "split"	, POS_SITTING , do_split    , 1, ADMLVL_IMMORT	, 0 },
  { "speak"    , "spe"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "spells"   , "spel"		, POS_RESTING , do_spells   , 0, ADMLVL_IMMORT	, 0 },
  { "stand"    , "st"		, POS_RESTING , do_stand    , 0, ADMLVL_NONE	, 0 },
  { "starbreaker", "starbr"     , POS_FIGHTING, do_breaker  , 0, ADMLVL_NONE  , 0 },
  { "stake"    , "stak"         , POS_SLEEPING, do_beacon   , 0, 0 , 0 },
  { "stat"     , "stat"		, POS_DEAD    , do_stat     , 0, ADMLVL_IMMORT	, 0 },
  { "status"   , "statu"        , POS_DEAD    , do_status   , 0, 0 , 0 },  
  { "steal"    , "ste"		, POS_STANDING, do_steal    , 1, ADMLVL_NONE	, 0 },
  { "stone"    , "ston"         , POS_STANDING, do_spit     , 0, ADMLVL_NONE    , 0 },
  { "stop"     , "sto"          , POS_STANDING, do_stop     , 0, ADMLVL_NONE    , 0 },
  { "study"    , "stu"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "summon"   , "summo"        , POS_STANDING, do_summon   , 0, ADMLVL_NONE    , 0 },
  { "sunder"   , "sunde"        , POS_STANDING, do_sunder   , 0, ADMLVL_NONE    , 0 },
  { "suppress" , "suppres"      , POS_STANDING, do_suppress , 0, ADMLVL_NONE    , 0 },
  { "swallow"  , "swall"        , POS_RESTING , do_use      , 0, ADMLVL_NONE  , SCMD_QUAFF },
  { "switch"   , "switch"	, POS_DEAD    , do_switch   , 0, ADMLVL_VICE	, 0 },
  { "syslog"   , "syslog"	, POS_DEAD    , do_syslog   , 0, ADMLVL_IMMORT	, 0 },

  /*{ "tcopy"    , "tcopy"  	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, SCMD_TEDIT },*/
  { "tailhide" , "tailh"  , POS_RESTING , do_tailhide       , 0, ADMLVL_NONE    , 0 },
  { "table"    , "tabl"         , POS_SITTING , do_table    , 0, ADMLVL_NONE    , 0 },
  { "teach"    , "teac"         , POS_STANDING, do_teach    , 0, ADMLVL_NONE    , 0 },
  { "tell"     , "tel"		, POS_DEAD    , do_tell     , 0, ADMLVL_NONE	, 0 },
  { "take"     , "tak"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "tailwhip" , "tailw"        , POS_FIGHTING, do_tailwhip , 0, ADMLVL_NONE    , 0 },
  { "taisha"   , "taish"        , POS_FIGHTING, do_taisha , 0, ADMLVL_NONE    , 0 },
  { "taste"    , "tas"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_TASTE },
  { "teleport" , "tele"		, POS_DEAD    , do_teleport , 0, ADMLVL_IMMORT	, 0 },
  { "telepathy", "telepa"       , POS_DEAD    , do_telepathy, 0, ADMLVL_NONE    , 0 },
  { "tedit"    , "tedit"	, POS_DEAD    , do_tedit    , 0, ADMLVL_GRGOD	, 0 },  
  { "test"     , "test"         , POS_DEAD    , do_gen_tog  , 0, ADMLVL_BUILDER    , SCMD_TEST },
  { "thaw"     , "thaw"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMMORT	, SCMD_THAW },
  { "think"    , "thin"         , POS_DEAD    , do_think    , 0, ADMLVL_NONE    , 0 },
  { "throw"    , "thro"         , POS_FIGHTING, do_throw    , 0, ADMLVL_NONE  , 0 },
  { "title"    , "title"	, POS_DEAD    , do_title    , 0, ADMLVL_NONE	, 0 },
  { "time"     , "time"		, POS_DEAD    , do_time     , 0, ADMLVL_NONE	, 0 },
  { "toggle"   , "toggle"	, POS_DEAD    , do_toggle   , 0, ADMLVL_NONE	, 0 },
  { "toplist"  , "toplis"       , POS_DEAD    , do_toplist  , 0, ADMLVL_NONE    , 0 },
  { "trackthru", "trackthru"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMPL	, SCMD_TRACK },
  { "train"    , "train"	, POS_STANDING, do_train    , 0, ADMLVL_NONE	, 0 },
  { "transfer" , "transfer"	, POS_SLEEPING, do_trans    , 0, ADMLVL_IMMORT	, 0 },
  { "transform", "transform"    , POS_FIGHTING, do_transform, 0, ADMLVL_NONE    , 0 },
  { "transo"   , "trans"        , POS_STANDING, do_transobj , 0, 5    , 0 },
  { "tribeam"  , "tribe"        , POS_FIGHTING, do_tribeam  , 0, ADMLVL_NONE    , 0},
  { "trigedit" , "trigedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_TRIGEDIT},
  { "trip"     , "trip"         , POS_FIGHTING, do_trip     , 0, ADMLVL_NONE    , 0},
  { "tsuihidan", "tsuihida"     , POS_FIGHTING, do_tsuihidan, 0, ADMLVL_NONE    , 0},
  { "tunnel"   , "tunne"        , POS_DEAD    , do_dig      , 0, ADMLVL_IMMORT  , 0 },
  { "twinslash", "twins"        , POS_FIGHTING, do_tslash   , 0, ADMLVL_NONE    , 0},
  { "twohand"  , "twohand"      , POS_DEAD    , do_twohand  , 0, ADMLVL_NONE    , 0},
  { "typo"     , "typo"		, POS_DEAD    , do_gen_write, 0, ADMLVL_NONE	, SCMD_TYPO },

  { "unlock"   , "unlock"	, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_UNLOCK },
  { "ungroup"  , "ungroup"	, POS_DEAD    , do_ungroup  , 0, ADMLVL_NONE	, 0 },
  { "unban"    , "unban"	, POS_DEAD    , do_unban    , 0, ADMLVL_GRGOD	, 0 },
  { "unaffect" , "unaffect"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_UNAFFECT },
  { "uppercut" , "upperc"       , POS_FIGHTING, do_uppercut , 0, ADMLVL_NONE   , 0 },
  { "upgrade"  , "upgrad"       , POS_RESTING , do_upgrade  , 0, ADMLVL_NONE    , 0 },
  { "uptime"   , "uptime"	, POS_DEAD    , do_date     , 0, ADMLVL_IMMORT	, SCMD_UPTIME },
  { "use"      , "use"		, POS_SITTING , do_use      , 1, ADMLVL_NONE	, SCMD_USE },
  { "users"    , "users"	, POS_DEAD    , do_users    , 0, ADMLVL_IMMORT	, 0 },

  { "value"    , "val"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "varstat"  , "varst"        , POS_DEAD    , do_varstat  , 0, ADMLVL_IMMORT  , 0 },
  { "version"  , "ver"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_VERSION },
  { "vieworder", "view" 	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_VIEWORDER },
  { "visible"  , "vis"		, POS_RESTING , do_visible  , 1, ADMLVL_NONE	, 0 },
  { "vnum"     , "vnum"		, POS_DEAD    , do_vnum     , 0, ADMLVL_IMMORT	, 0 },
  { "voice"    , "voic"         , POS_RESTING , do_voice    , 0, ADMLVL_NONE  , 0 },
  { "vstat"    , "vstat"	, POS_DEAD    , do_vstat    , 0, ADMLVL_IMMORT	, 0 },

  { "wake"     , "wa"		, POS_SLEEPING, do_wake     , 0, ADMLVL_NONE	, 0 },
  { "warppool" , "warppoo"      , POS_STANDING, do_warppool , 0, ADMLVL_NONE    , 0 },
  { "waterrazor", "waterraz"    , POS_STANDING, do_razor    , 0, ADMLVL_NONE    , 0 },
  { "waterspikes", "waterspik"  , POS_STANDING, do_spike    , 0, ADMLVL_NONE    , 0 },
  { "wear"     , "wea"		, POS_RESTING , do_wear     , 0, ADMLVL_NONE	, 0 },
  { "weather"  , "weather"	, POS_RESTING , do_weather  , 0, ADMLVL_NONE	, 0 },
  { "who"      , "who"		, POS_DEAD    , do_who      , 0, ADMLVL_NONE	, 0 },
  { "whoami"   , "whoami"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WHOAMI },
  { "whohide"  , "whohide"      , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_WHOHIDE },
  { "whois"    , "whois"	, POS_DEAD    , do_whois    , 0, ADMLVL_NONE	, 0 },
  { "where"    , "where"	, POS_RESTING , do_where    , 1, ADMLVL_IMMORT	, 0 },
  { "whisper"  , "whisper"	, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_WHISPER },
  { "wield"    , "wie"		, POS_RESTING , do_wield    , 0, ADMLVL_NONE	, 0 },
  { "will"     , "wil"          , POS_RESTING , do_willpower, 0, ADMLVL_NONE    , 0 },
  { "wimpy"    , "wimpy"	, POS_DEAD    , do_value    , 0, ADMLVL_NONE	, SCMD_WIMPY },
  { "withdraw" , "withdraw"	, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "wire"     , "wir"          , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "wiznet"   , "wiz"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_IMMORT	, 0 },
  { ";"        , ";"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_IMMORT	, 0 },
  { "wizhelp"  , "wizhelp"	, POS_SLEEPING, do_commands , 0, ADMLVL_IMMORT	, SCMD_WIZHELP },
  { "wizlist"  , "wizlist"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WIZLIST },
  { "wizlock"  , "wizlock"	, POS_DEAD    , do_wizlock  , 0, ADMLVL_IMMORT	, 0 },
  { "wizupdate", "wizupdate"    , POS_DEAD    , do_wizupdate, 0, ADMLVL_IMPL	, 0 },
  { "write"    , "write"	, POS_STANDING, do_write    , 1, ADMLVL_NONE	, 0 },


  { "zanzoken" , "zanzo"        , POS_FIGHTING, do_zanzoken , 0, ADMLVL_NONE, 0 },
  { "zen"      , "ze"           , POS_FIGHTING, do_zen      , 0, ADMLVL_NONE, 0 },
  { "zcheck"   , "zcheck"       , POS_DEAD    , do_zcheck   , 0, ADMLVL_GOD, 0 },
  { "zreset"   , "zreset"	, POS_DEAD    , do_zreset   , 0, ADMLVL_IMMORT	, 0 },
  { "zedit"    , "zedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_ZEDIT },
  { "zlist"    , "zlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_ZLIST },
  { "zpurge"   , "zpurge"       , POS_DEAD    , do_zpurge   , 0, ADMLVL_GRGOD, 0 },

  /* DG trigger commands */
  { "attach"   , "attach"	, POS_DEAD    , do_attach   , 0, ADMLVL_BUILDER	, 0 },
  { "detach"   , "detach"	, POS_DEAD    , do_detach   , 0, ADMLVL_BUILDER	, 0 },
  { "detect"   , "detec"        , POS_STANDING, do_radar    , 0, ADMLVL_NONE     , 0 },
  { "tlist"    , "tlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_TLIST },
  { "tstat"    , "tstat"	, POS_DEAD    , do_tstat    , 0, ADMLVL_IMMORT	, 0 },
  { "masound"  , "masound"	, POS_DEAD    , do_masound  , -1, ADMLVL_NONE	, 0 },
  { "mheal"    , "mhea"         , POS_SITTING , do_mheal    , -1, ADMLVL_NONE   , 0 },
  { "mkill"    , "mkill"	, POS_STANDING, do_mkill    , -1, ADMLVL_NONE	, 0 },
  { "mjunk"    , "mjunk"	, POS_SITTING , do_mjunk    , -1, ADMLVL_NONE	, 0 },
  { "mdamage"  , "mdamage"	, POS_DEAD    , do_mdamage  , -1, ADMLVL_NONE	, 0 },
  { "mdoor"    , "mdoor"	, POS_DEAD    , do_mdoor    , -1, ADMLVL_NONE	, 0 },
  { "mecho"    , "mecho"	, POS_DEAD    , do_mecho    , -1, ADMLVL_NONE	, 0 },
  { "mechoaround", "mechoaround", POS_DEAD    , do_mechoaround, -1, ADMLVL_NONE	, 0 },
  { "msend"    , "msend"	, POS_DEAD    , do_msend    , -1, ADMLVL_NONE	, 0 },
  { "mload"    , "mload"	, POS_DEAD    , do_mload    , -1, ADMLVL_NONE	, 0 },
  { "mpurge"   , "mpurge"	, POS_DEAD    , do_mpurge   , -1, ADMLVL_NONE	, 0 },
  { "mgoto"    , "mgoto"	, POS_DEAD    , do_mgoto    , -1, ADMLVL_NONE	, 0 },
  { "mat"      , "mat"		, POS_DEAD    , do_mat      , -1, ADMLVL_NONE	, 0 },
  { "mteleport", "mteleport"	, POS_DEAD    , do_mteleport, -1, ADMLVL_NONE	, 0 },
  { "mforce"   , "mforce"	, POS_DEAD    , do_mforce   , -1, ADMLVL_NONE	, 0 },
  { "mremember", "mremember"	, POS_DEAD    , do_mremember, -1, ADMLVL_NONE	, 0 },
  { "mforget"  , "mforget"	, POS_DEAD    , do_mforget  , -1, ADMLVL_NONE	, 0 },
  { "mtransform", "mtransform"	, POS_DEAD    , do_mtransform, -1, ADMLVL_NONE	, 0 },
  { "mzoneecho", "mzoneecho"	, POS_DEAD    , do_mzoneecho, -1, ADMLVL_NONE	, 0 },
  { "vdelete"  , "vdelete"	, POS_DEAD    , do_vdelete  , 0, ADMLVL_BUILDER	, 0 },
  { "mfollow"  , "mfollow"	, POS_DEAD    , do_mfollow  , -1, ADMLVL_NONE	, 0 },

  { "\n", "zzzzzzz", 0, 0, 0, ADMLVL_NONE	, 0 } };	/* this must be last */




int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
    if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
      if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}

  /*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  int skip_ld = 0;
  char *line;
  char arg[MAX_INPUT_LENGTH];

  switch (GET_POS(ch)) {
    case POS_DEAD:
    case POS_INCAP:
    case POS_MORTALLYW:
    case POS_STUNNED:
      GET_POS(ch) = POS_SITTING;
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
  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. 
   */
  /* otherwise, find the command */
  {
  int cont;                                            /* continue the command checks */
  cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
  if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
  if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
  if (cont) return;                                    /* yes, command trigger took over */
  }
  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++) {
    if (!strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level &&
          GET_ADMLEVEL(ch) >= complete_cmd_info[cmd].minimum_admlevel)
	break;
  }

  char blah[MAX_INPUT_LENGTH];

  sprintf(blah, "%s", complete_cmd_info[cmd].command);
  if (!strcasecmp(blah, "throw"))
      ch->throws = rand_number(1, 3);


  if (*complete_cmd_info[cmd].command == '\n') {
      send_to_char(ch, "Huh!?!\r\n");
      return;
  }

  else if (!command_pass(blah, ch) && GET_ADMLEVEL(ch) < 1)
      send_to_char(ch, "It's unfortunate...\r\n");
  else if (check_disabled(&complete_cmd_info[cmd]))    /* is it disabled? */
      send_to_char(ch, "This command has been temporarily disabled.\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_GOOP) && GET_ADMLEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You only have your internal thoughts until your body has finished regenerating!\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_ADMLEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_SPIRAL))
    send_to_char(ch, "You are occupied with your Spiral Comet attack!\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_admlevel >= ADMLVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position && GET_POS(ch) != POS_FIGHTING) {
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
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
     ((*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, complete_cmd_info[cmd].subcmd));
    } 
   }
}