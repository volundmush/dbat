#pragma once
#include "Command.h"

struct Object;
struct Character;

constexpr int ALIAS_SIMPLE = 0;
constexpr int ALIAS_COMPLEX = 1;

#define ALIAS_SEP_CHAR ';'
#define ALIAS_VAR_CHAR '$'
#define ALIAS_GLOB_CHAR '*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
constexpr int SCMD_NORTH = 1;
constexpr int SCMD_EAST = 2;
constexpr int SCMD_SOUTH = 3;
constexpr int SCMD_WEST = 4;
constexpr int SCMD_UP = 5;
constexpr int SCMD_DOWN = 6;
constexpr int SCMD_NW = 7;
constexpr int SCMD_NE = 8;
constexpr int SCMD_SE = 9;
constexpr int SCMD_SW = 10;
constexpr int SCMD_IN = 11;
constexpr int SCMD_OUT = 12;

/* do_gen_ps */
constexpr int SCMD_INFO = 0;
constexpr int SCMD_HANDBOOK = 1;
constexpr int SCMD_CREDITS = 2;
constexpr int SCMD_NEWS = 3;
constexpr int SCMD_WIZLIST = 4;
constexpr int SCMD_POLICIES = 5;
constexpr int SCMD_VERSION = 6;
constexpr int SCMD_IMMLIST = 7;
constexpr int SCMD_MOTD = 8;
constexpr int SCMD_IMOTD = 9;
constexpr int SCMD_CLEAR = 10;
constexpr int SCMD_WHOAMI = 11;

/* do_gen_tog */
constexpr int SCMD_NOSUMMON = 0;
constexpr int SCMD_NOHASSLE = 1;
constexpr int SCMD_BRIEF = 2;
constexpr int SCMD_COMPACT = 3;
constexpr int SCMD_NOTELL = 4;
constexpr int SCMD_NOAUCTION = 5;
constexpr int SCMD_DEAF = 6;
constexpr int SCMD_NOGOSSIP = 7;
constexpr int SCMD_NOGRATZ = 8;
constexpr int SCMD_NOWIZ = 9;
constexpr int SCMD_QUEST = 10;
constexpr int SCMD_ROOMFLAGS = 11;
constexpr int SCMD_NOREPEAT = 12;
constexpr int SCMD_HOLYLIGHT = 13;
constexpr int SCMD_SLOWNS = 14;
constexpr int SCMD_AUTOEXIT = 15;
constexpr int SCMD_TRACK = 16;
constexpr int SCMD_BUILDWALK = 17;
constexpr int SCMD_AFK = 18;
constexpr int SCMD_AUTOASSIST = 19;
constexpr int SCMD_AUTOLOOT = 20;
constexpr int SCMD_AUTOGOLD = 21;
constexpr int SCMD_CLS = 22;
constexpr int SCMD_AUTOSPLIT = 23;
constexpr int SCMD_AUTOSAC = 24;
constexpr int SCMD_SNEAK = 25;
constexpr int SCMD_HIDE = 26;
constexpr int SCMD_AUTOMEM = 27;
constexpr int SCMD_VIEWORDER = 28;
constexpr int SCMD_NOCOMPRESS = 29;
constexpr int SCMD_TEST = 30;
constexpr int SCMD_WHOHIDE = 31;
constexpr int SCMD_NMWARN = 32;
constexpr int SCMD_HINTS = 33;
constexpr int SCMD_NODEC = 34;
constexpr int SCMD_NOEQSEE = 35;
constexpr int SCMD_NOMUSIC = 36;
constexpr int SCMD_NOPARRY = 37;
constexpr int SCMD_LKEEP = 38;
constexpr int SCMD_CARVE = 39;
constexpr int SCMD_NOGIVE = 40;
constexpr int SCMD_INSTRUCT = 41;
constexpr int SCMD_GHEALTH = 42;
constexpr int SCMD_IHEALTH = 43;

/* do_wizutil */
constexpr int SCMD_REROLL = 0;
constexpr int SCMD_PARDON = 1;
constexpr int SCMD_NOTITLE = 2;
constexpr int SCMD_SQUELCH = 3;
constexpr int SCMD_FREEZE = 4;
constexpr int SCMD_THAW = 5;
constexpr int SCMD_UNAFFECT = 6;

/* do_spec_com */
constexpr int SCMD_WHISPER = 0;
constexpr int SCMD_ASK = 1;

/* do_gen_com */
constexpr int SCMD_HOLLER = 0;
constexpr int SCMD_SHOUT = 1;
constexpr int SCMD_GOSSIP = 2;
constexpr int SCMD_AUCTION = 3;
constexpr int SCMD_GRATZ = 4;
constexpr int SCMD_GEMOTE = 5;

/* do_shutdown */
constexpr int SCMD_SHUTDOW = 0;
constexpr int SCMD_SHUTDOWN = 1;

/* do_quit */
constexpr int SCMD_QUI = 0;
constexpr int SCMD_QUIT = 1;

/* do_date */
constexpr int SCMD_DATE = 0;
constexpr int SCMD_UPTIME = 1;

/* do_commands */
constexpr int SCMD_COMMANDS = 0;
constexpr int SCMD_SOCIALS = 1;
constexpr int SCMD_WIZHELP = 2;

/* do_drop */
constexpr int SCMD_DROP = 0;
constexpr int SCMD_JUNK = 1;
constexpr int SCMD_DONATE = 2;

/* do_gen_write */
constexpr int SCMD_BUG = 0;
constexpr int SCMD_TYPO = 1;
constexpr int SCMD_IDEA = 2;

/* do_look */
constexpr int SCMD_LOOK = 0;
constexpr int SCMD_READ = 1;
constexpr int SCMD_SEARCH = 2;

/* do_qcomm */
constexpr int SCMD_QSAY = 0;
constexpr int SCMD_QECHO = 1;

/* do_pour */
constexpr int SCMD_POUR = 0;
constexpr int SCMD_FILL = 1;

/* do_poof */
constexpr int SCMD_POOFIN = 0;
constexpr int SCMD_POOFOUT = 1;

/* do_hit */
constexpr int SCMD_HIT = 0;
constexpr int SCMD_MURDER = 1;

/* do_eat */
constexpr int SCMD_EAT = 0;
constexpr int SCMD_TASTE = 1;
constexpr int SCMD_DRINK = 2;
constexpr int SCMD_SIP = 3;

/* do_use */
constexpr int SCMD_USE = 0;
constexpr int SCMD_QUAFF = 1;
constexpr int SCMD_RECITE = 2;

/* do_echo */
constexpr int SCMD_ECHO = 0;
constexpr int SCMD_EMOTE = 1;
constexpr int SCMD_SMOTE = 2;

/* do_gen_door */
constexpr int SCMD_OPEN = 0;
constexpr int SCMD_CLOSE = 1;
constexpr int SCMD_UNLOCK = 2;
constexpr int SCMD_LOCK = 3;
constexpr int SCMD_PICK = 4;

/* do_olc */
constexpr int SCMD_OASIS_REDIT = 0;
constexpr int SCMD_OASIS_OEDIT = 1;
constexpr int SCMD_OASIS_ZEDIT = 2;
constexpr int SCMD_OASIS_MEDIT = 3;
constexpr int SCMD_OASIS_SEDIT = 4;
constexpr int SCMD_OASIS_CEDIT = 5;
constexpr int SCMD_OLC_SAVEINFO = 7;
constexpr int SCMD_OASIS_RLIST = 8;
constexpr int SCMD_OASIS_MLIST = 9;
constexpr int SCMD_OASIS_OLIST = 10;
constexpr int SCMD_OASIS_SLIST = 11;
constexpr int SCMD_OASIS_ZLIST = 12;
constexpr int SCMD_OASIS_TRIGEDIT = 13;
constexpr int SCMD_OASIS_AEDIT = 14;
constexpr int SCMD_OASIS_TLIST = 15;
constexpr int SCMD_OASIS_LINKS = 16;
constexpr int SCMD_OASIS_GEDIT = 17;
constexpr int SCMD_OASIS_GLIST = 18;
constexpr int SCMD_OASIS_HEDIT = 19;
constexpr int SCMD_OASIS_HSEDIT = 20;

/* do_builder_list */

constexpr int SCMD_RLIST = 0;
constexpr int SCMD_OLIST = 1;
constexpr int SCMD_MLIST = 2;
constexpr int SCMD_TLIST = 3;
constexpr int SCMD_SLIST = 4;
constexpr int SCMD_GLIST = 5;

/* * do_assemble * These constants *must* corespond with
     the ASSM_xxx constants in * assemblies.h. */
constexpr int SCMD_MAKE = 0;
constexpr int SCMD_BAKE = 1;
constexpr int SCMD_BREW = 2;
constexpr int SCMD_ASSEMBLE = 3;
constexpr int SCMD_CRAFT = 4;
constexpr int SCMD_FLETCH = 5;
constexpr int SCMD_KNIT = 6;
constexpr int SCMD_MIX = 7;
constexpr int SCMD_THATCH = 8;
constexpr int SCMD_WEAVE = 9;
constexpr int SCMD_FORGE = 10;

constexpr int SCMD_MEMORIZE = 1;
constexpr int SCMD_FORGET = 2;
constexpr int SCMD_STOP = 3;
constexpr int SCMD_WHEN_SLOT = 4;

/* do_value list */
constexpr int SCMD_WIMPY = 0;
constexpr int SCMD_POWERATT = 1;
constexpr int SCMD_COMBATEXP = 2;

/* do_cast */
constexpr int SCMD_CAST = 0;
constexpr int SCMD_ART = 1;

/* oasis_copy */
constexpr int SCMD_TEDIT = 0;
constexpr int SCMD_REDIT = 1;
constexpr int SCMD_OEDIT = 2;
constexpr int SCMD_MEDIT = 3;

struct command_info
{
    const char *command;
    const char *sort_as;
    int8_t minimum_position;
    CommandFunc command_pointer;
    int16_t minimum_level;
    int16_t minimum_admlevel;
    int subcmd;
    int wait_list;
};

extern const struct command_info cmd_info[];
extern struct command_info *complete_cmd_info;

extern int matchCommand(Character* ch, std::string_view cmd);

extern DECCMD(do_oasis_list);
extern DECCMD(do_oasis_links);
extern DECCMD(do_oasis_oedit);
extern DECCMD(do_oasis_redit);
extern DECCMD(do_oasis_sedit);
extern DECCMD(do_oasis_gedit);
extern DECCMD(do_oasis_zedit);
extern DECCMD(do_oasis_cedit);
extern DECCMD(do_oasis_trigedit);
extern DECCMD(do_oasis_aedit);
extern DECCMD(do_oasis_hedit);
extern DECCMD(do_oasis_medit);
extern DECCMD(do_skillset);
extern DECCMD(do_track);
extern DECCMD(do_kousengan);

extern DECCMD(do_heal);

extern DECCMD(do_trip);

extern DECCMD(do_koteiru);

extern DECCMD(do_razor);

extern DECCMD(do_spike);

extern DECCMD(do_ddslash);

extern DECCMD(do_kakusanha);

extern DECCMD(do_psyblast);

extern DECCMD(do_punch);

extern DECCMD(do_powerup);

extern DECCMD(do_srepair);

extern DECCMD(do_absorb);

extern DECCMD(do_kaioken);

extern DECCMD(do_strike);

extern DECCMD(do_kick);

extern DECCMD(do_elbow);

extern DECCMD(do_knee);

extern DECCMD(do_uppercut);

extern DECCMD(do_roundhouse);

extern DECCMD(do_heeldrop);

extern DECCMD(do_slam);

extern DECCMD(do_tailwhip);

extern DECCMD(do_head);

extern DECCMD(do_bite);

extern DECCMD(do_ram);

extern DECCMD(do_breath);

extern DECCMD(do_kiball);

extern DECCMD(do_kiblast);

extern DECCMD(do_beam);

extern DECCMD(do_renzo);

extern DECCMD(do_tsuihidan);

extern DECCMD(do_shogekiha);

extern DECCMD(do_kamehameha);

extern DECCMD(do_galikgun);

extern DECCMD(do_masenko);

extern DECCMD(do_hellspear);

extern DECCMD(do_hellflash);

extern DECCMD(do_dualbeam);

extern DECCMD(do_honoo);

extern DECCMD(do_pbarrage);

extern DECCMD(do_tslash);

extern DECCMD(do_pslash);

extern DECCMD(do_crusher);

extern DECCMD(do_eraser);

extern DECCMD(do_spiral);

extern DECCMD(do_tribeam);

extern DECCMD(do_dodonpa);

extern DECCMD(do_hass);

extern DECCMD(do_zanzoken);

extern DECCMD(do_deathball);

extern DECCMD(do_deathbeam);

extern DECCMD(do_kienzan);

extern DECCMD(do_bigbang);

extern DECCMD(do_final);

extern DECCMD(do_sbc);

extern DECCMD(do_scatter);

extern DECCMD(do_nova);

extern DECCMD(do_breaker);

extern DECCMD(do_seishou);

extern DECCMD(do_ensnare);

extern DECCMD(do_barrier);

extern DECCMD(do_attack);

extern DECCMD(do_stand);

extern DECCMD(do_fly);

extern DECCMD(do_wake);

extern DECCMD(do_flee);

extern DECCMD(do_get);

extern DECCMD(do_split);

extern DECCMD(do_sac);

extern DECCMD(do_transform);

extern DECCMD(do_escape);

extern DECCMD(do_balefire);

extern DECCMD(do_blessedhammer);

extern DECCMD(do_reboot);

extern DECCMD(do_action);

extern DECCMD(do_insult);

extern DECCMD(do_gmote);

extern DECCMD(do_gen_comm);

extern DECCMD(do_charge);

extern DECCMD(do_wear);

extern DECCMD(do_quit);

extern DECCMD(do_save);

extern DECCMD(do_not_here);

extern DECCMD(do_hide);

extern DECCMD(do_steal);

extern DECCMD(do_practice);

extern DECCMD(do_visible);

extern DECCMD(do_title);

extern DECCMD(do_group);

extern DECCMD(do_ungroup);

extern DECCMD(do_report);

extern DECCMD(do_use);

extern DECCMD(do_value);

extern DECCMD(do_display);

extern DECCMD(do_gen_write);

extern DECCMD(do_gen_tog);

extern DECCMD(do_file);

extern DECCMD(do_scribe);

extern DECCMD(do_scouter);

extern DECCMD(do_snet);

extern DECCMD(do_spar);

extern DECCMD(do_pushup);

extern DECCMD(do_situp);

extern DECCMD(do_summon);

extern DECCMD(do_instant);

extern DECCMD(do_solar);

extern DECCMD(do_eyec);

extern DECCMD(do_eavesdrop);

extern DECCMD(do_disguise);

extern DECCMD(do_appraise);

extern DECCMD(do_forgery);

extern DECCMD(do_plant);

extern DECCMD(do_focus);

extern DECCMD(do_regenerate);

extern DECCMD(do_ingest);

extern DECCMD(do_upgrade);

extern DECCMD(do_recharge);

extern DECCMD(do_form);

extern DECCMD(do_spit);

extern DECCMD(do_majinize);

extern DECCMD(do_potential);

extern DECCMD(do_telepathy);

extern DECCMD(do_fury);

extern DECCMD(do_pose);

extern DECCMD(do_implant);

extern DECCMD(do_suppress);

extern DECCMD(do_drag);

extern DECCMD(do_stop);

extern DECCMD(do_future);

extern DECCMD(do_candy);

extern DECCMD(do_kura);

extern DECCMD(do_taisha);

extern DECCMD(do_paralyze);

extern DECCMD(do_infuse);

extern DECCMD(do_rip);

extern DECCMD(do_train);

extern DECCMD(do_grapple);

extern DECCMD(do_willpower);

extern DECCMD(do_commune);

extern DECCMD(do_rpp);

extern DECCMD(do_meditate);

extern DECCMD(do_aura);

extern DECCMD(do_think);

extern DECCMD(do_block);

extern DECCMD(do_compare);

extern DECCMD(do_break);

extern DECCMD(do_fix);

extern DECCMD(do_resurrect);

extern DECCMD(do_aid);

extern DECCMD(do_assist);

extern DECCMD(do_kill);

extern DECCMD(do_attack2);

extern DECCMD(do_rogafufuken);

extern DECCMD(do_baku);

extern DECCMD(do_spiritball);

extern DECCMD(do_genki);

extern DECCMD(do_geno);

extern DECCMD(do_rescue);

extern DECCMD(do_gen_door);

extern DECCMD(do_enter);

extern DECCMD(do_leave);

extern DECCMD(do_sit);

extern DECCMD(do_rest);

extern DECCMD(do_sleep);

extern DECCMD(do_follow);

extern DECCMD(do_carry);

extern DECCMD(do_land);

extern DECCMD(do_move);

extern DECCMD(do_spoil);

extern DECCMD(do_feed);

extern DECCMD(do_beacon);

extern DECCMD(do_dimizu);

extern DECCMD(do_obstruct);

extern DECCMD(do_warppool);

extern DECCMD(do_fireshield);

extern DECCMD(do_cook);

extern DECCMD(do_adrenaline);

extern DECCMD(do_arena);

extern DECCMD(do_bury);

extern DECCMD(do_hayasa);

extern DECCMD(do_instill);

extern DECCMD(do_kanso);

extern DECCMD(do_hydromancy);

extern DECCMD(do_channel);

extern DECCMD(do_shimmer);

extern DECCMD(do_metamorph);

extern DECCMD(do_amnisiac);

extern DECCMD(do_healglow);

extern DECCMD(do_resize);

extern DECCMD(do_scry);

extern DECCMD(do_runic);

extern DECCMD(do_extract);

extern DECCMD(do_fish);

extern DECCMD(do_defend);

extern DECCMD(do_lifeforce);

extern DECCMD(do_liquefy);

extern DECCMD(do_shell);

extern DECCMD(do_moondust);

extern DECCMD(do_preference);

extern DECCMD(do_song);

extern DECCMD(do_multiform);

extern DECCMD(do_spiritcontrol);

extern DECCMD(do_ashcloud);

extern DECCMD(do_silk);

extern DECCMD(do_auction);

extern DECCMD(do_bid);

extern DECCMD(do_assemble);

extern DECCMD(do_remove);

extern DECCMD(do_put);

extern DECCMD(do_drop);

extern DECCMD(do_give);

extern DECCMD(do_drink);

extern DECCMD(do_eat);

extern DECCMD(do_pour);

extern DECCMD(do_wield);

extern DECCMD(do_grab);

extern DECCMD(do_twohand);

extern DECCMD(do_deploy);

extern DECCMD(do_pack);

extern DECCMD(do_garden);

extern DECCMD(do_refuel);

extern DECCMD(do_desc);

extern DECCMD(do_look);

extern DECCMD(do_examine);

extern DECCMD(do_gold);

extern DECCMD(do_score);

extern DECCMD(do_status);

extern DECCMD(do_inventory);

extern DECCMD(do_equipment);

extern DECCMD(do_time);

extern DECCMD(do_weather);

extern DECCMD(do_help);

extern DECCMD(do_who);

extern DECCMD(do_users);

extern DECCMD(do_gen_ps);

extern DECCMD(do_where);

extern DECCMD(do_levels);

extern DECCMD(do_consider);

extern DECCMD(do_diagnose);

extern DECCMD(do_color);

extern DECCMD(do_toggle);

extern DECCMD(do_commands);

extern DECCMD(do_exits);

extern DECCMD(do_autoexit);

extern DECCMD(do_history);

extern DECCMD(do_map);

extern DECCMD(do_rptrans);

extern DECCMD(do_finger);

extern DECCMD(do_perf);

extern DECCMD(do_nickname);

extern DECCMD(do_table);

extern DECCMD(do_play);

extern DECCMD(do_post);

extern DECCMD(do_hand);

extern DECCMD(do_shuffle);

extern DECCMD(do_draw);

extern DECCMD(do_kyodaika);

extern DECCMD(do_mimic);

extern DECCMD(do_rdisplay);

extern DECCMD(do_evolve);

extern DECCMD(do_showoff);

extern DECCMD(do_intro);

extern DECCMD(do_scan);

extern DECCMD(do_toplist);

extern DECCMD(do_whois);

extern DECCMD(do_oaffects);

extern DECCMD(do_warp);

extern DECCMD(do_drive);

extern DECCMD(do_ship_fire);

extern DECCMD(do_dig);

extern DECCMD(do_echo);

extern DECCMD(do_send);

extern DECCMD(do_at);

extern DECCMD(do_goto);

extern DECCMD(do_trans);

extern DECCMD(do_teleport);

extern DECCMD(do_vnum);

extern DECCMD(do_stat);

extern DECCMD(do_shutdown);

extern DECCMD(do_recall);

extern DECCMD(do_snoop);

extern DECCMD(do_switch);

extern DECCMD(do_return);

extern DECCMD(do_load);

extern DECCMD(do_vstat);

extern DECCMD(do_purge);

extern DECCMD(do_syslog);

extern DECCMD(do_advance);

extern DECCMD(do_restore);

extern DECCMD(do_invis);

extern DECCMD(do_gecho);

extern DECCMD(do_poofset);

extern DECCMD(do_dc);

extern DECCMD(do_wizlock);

extern DECCMD(do_date);

extern DECCMD(do_last);

extern DECCMD(do_force);

extern DECCMD(do_wiznet);

extern DECCMD(do_zreset);

extern DECCMD(do_wizutil);

extern DECCMD(do_show);

extern DECCMD(do_set);

extern DECCMD(do_saveall);

extern DECCMD(do_wizupdate);

extern DECCMD(do_chown);

extern DECCMD(do_zpurge);

extern DECCMD(do_zcheck);

extern DECCMD(do_checkloadstatus);

extern DECCMD(do_spells);

extern DECCMD(do_finddoor);

extern DECCMD(do_interest);

extern DECCMD(do_transobj);

extern DECCMD(do_permission);

extern DECCMD(do_reward);

extern DECCMD(do_approve);

extern DECCMD(do_newsedit);

extern DECCMD(do_news);

extern DECCMD(do_lag);

extern DECCMD(do_hell);

extern DECCMD(do_varstat);

extern DECCMD(do_handout);

extern DECCMD(do_ginfo);

extern DECCMD(do_plist);

extern DECCMD(do_peace);

extern DECCMD(do_raise);

extern DECCMD(do_boom);

// For administrating Zones.
extern DECCMD(do_mush_zone);
// for administrating reset commands.
extern DECCMD(do_mush_reset);
// For administrating room flags.
// for administrating exits/directions.
extern DECCMD(do_mush_exit);
// for administrating other fields...
extern DECCMD(do_mush_location);
extern DECCMD(do_mush_choices);
extern DECCMD(do_mush_room);
extern DECCMD(do_mush_area);
extern DECCMD(do_mush_structure);

extern DECCMD(do_say);

extern DECCMD(do_gsay);

extern DECCMD(do_tell);

extern DECCMD(do_reply);

extern DECCMD(do_respond);

extern DECCMD(do_spec_comm);

extern DECCMD(do_write);

extern DECCMD(do_page);

extern DECCMD(do_qcomm);

extern DECCMD(do_voice);

extern DECCMD(do_languages);

extern DECCMD(do_osay);

extern DECCMD(do_alias);
extern DECCMD(do_bash);
extern DECCMD(do_combine);
extern DECCMD(do_combo);
extern DECCMD(do_copyover);
extern DECCMD(do_energize);
extern DECCMD(do_findkey);
extern DECCMD(do_hspiral);
extern DECCMD(do_lightgrenade);
extern DECCMD(do_malice);
extern DECCMD(do_nogrow);
extern DECCMD(do_pgrant);
extern DECCMD(do_rpreward);
extern DECCMD(do_eratime);
extern DECCMD(do_sradar);
extern DECCMD(do_restring);
extern DECCMD(do_rcopy);
extern DECCMD(do_selfd);
extern DECCMD(do_skills);
extern DECCMD(do_sunder);
extern DECCMD(do_tailhide);
extern DECCMD(do_teach);
extern DECCMD(do_throw);
extern DECCMD(do_zen);
extern DECCMD(do_attach);
extern DECCMD(do_detach);
extern DECCMD(do_radar);
extern DECCMD(do_tstat);
extern DECCMD(do_masound);
extern DECCMD(do_mheal);
extern DECCMD(do_mkill);
extern DECCMD(do_mjunk);
extern DECCMD(do_mdamage);
extern DECCMD(do_mdoor);
extern DECCMD(do_mecho);
extern DECCMD(do_mechoaround);
extern DECCMD(do_msend);
extern DECCMD(do_mload);
extern DECCMD(do_mpurge);
extern DECCMD(do_mgoto);
extern DECCMD(do_mat);
extern DECCMD(do_mteleport);
extern DECCMD(do_mforce);
extern DECCMD(do_mremember);
extern DECCMD(do_mforget);
extern DECCMD(do_mtransform);
extern DECCMD(do_mzoneecho);
extern DECCMD(do_vdelete);
extern DECCMD(do_mfollow);
extern DECCMD(do_maddtransform);