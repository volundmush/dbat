/* who.c: who, users, where, finger, whois, toplist
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <cstdlib>

#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/time.hpp"
#include "dbat/help.hpp"
#include "dbat/command.hpp"
#include "dbat/flags.hpp"
#include "dbat/affect.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/races.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.social.hpp"
#include "dbat/maputils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/boards.hpp"
#include "dbat/screen.hpp"
#include "dbat/mail.hpp"
#include "dbat/guild.hpp"
#include "dbat/disabled.hpp"
#include "dbat/game_world.hpp"

#include "informative_internal.hpp"

static void print_object_location(GameWorld *g, int num, struct obj_data *obj, struct char_data *ch, int recur);
static void perform_mortal_where(GameWorld *g, struct char_data *ch, char *arg);
static void perform_immort_where(GameWorld *g, struct char_data *ch, char *arg);

ACMD(do_finger)
{

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "What user are you wanting to look at?\r\n");
    return;
  }
  if (!readUserIndex(arg)) {
    send_to_char(g, ch, "That user does not exist\r\n");
    return;
  }
  fingerUser(g, ch, arg);
  return;
}

/* Written by Rhade */
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  int i, num_can_see = 0;
  char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int low = 0, high = CONFIG_LEVEL_CAP, localwho = 0, questwho = 0, hide = 0;
  int showclass = 0, short_list = 0, outlaws = 0;
  int who_room = 0, showgroup = 0, showleader = 0;
  const char *line_color = "@n";

  skip_spaces(&argument);
  strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  struct {
    const char *disp{};
    int min_level{};
    int max_level{};
    int count{}; /* must always start as 0 */
  } rank[] = {
      {"\r\n               @c------------  @D[    @gI@Gm@Wm@Do@Gr@Dt@Wa@Gl@gs   @D]  @c------------@n\r\n", ADMLVL_IMMORT,     ADMLVL_IMPL,       0},
      {"\r\n@D[@wx@D]@yxxxxxxxxxx@W  [    @GImmortals   @W]  @yxxxxxxxxxx@D[@wx@D]@n\r\n",                  ADMLVL_IMMORT + 8, ADMLVL_GRGOD + 8,  0},
      {"\r\n               @c------------  @D[     @DM@ro@Rr@wt@Ra@rl@Ds    ]  @c------------@n\r\n",       0,                 ADMLVL_IMMORT - 1, 0}
      /*{ "\r\n@GAdministrators@n\r\n\r\n", ADMLVL_GRGOD, ADMLVL_IMPL, 0},
    { "\r\n@GImmortals@n\r\n\r\n"     , ADMLVL_IMMORT, ADMLVL_GRGOD - 1, 0},
    { "\r\n@GMortal@n\r\n\r\n"        , 0, ADMLVL_IMMORT - 1, 0 }*/
  };
  char *tmstr;
  tmstr                        = asctime(localtime(&g->PCOUNTDATE));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  int num_ranks = countof(rank);
  send_to_char(g, ch, "\r\n      @r{@b===============  @D[  @DD@wr@ca@Cg@Y(@R*@Y)@Wn@cB@Da@cl@Cl @DA@wd@cv@Ce@Wnt @DT@wr@cu@Ct@Wh@n  @D]  @b===============@r}      @n\r\n");
  for (d = g->descriptors; d && !short_list; d = d->next) {
    if (!desc_is_playing(d))
      continue;
    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    line_color = "@w";

    if (char_can_see(g, ch, tch) && desc_is_playing(d)) {
      if (*name_search && strcasecmp(char_get_name(tch), name_search) &&
          !strstr(char_get_title(tch), name_search))
        continue;
      if (!char_can_see(g, ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) && !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (questwho && !char_prf_flagged(g, tch, PRF_QUEST))
        continue;
      if (localwho && rroom_get_zone(g, char_get_in_room(ch)) != rroom_get_zone(g, char_get_in_room(tch)))
        continue;
      if (char_prf_flagged(g, tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT) {
        hide += 1;
        continue;
      }
      if (who_room && (char_get_in_room(tch) != char_get_in_room(ch)))
        continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
        continue;
      if (showgroup && (!char_get_master(tch) || !char_aff_flagged(tch, AFF_GROUP)))
        continue;
      for (i = 0; i < num_ranks; i++)
        if (char_get_admlevel(tch) >= rank[i].min_level && char_get_admlevel(tch) <= rank[i].max_level)
          rank[i].count++;
    }
  }

  for (i = 0; i < num_ranks; i++) {
    if (!rank[i].count && !short_list)
      continue;

    if (short_list)
      send_to_char(g, ch, "Players\r\n-------\r\n");
    else
      send_to_char(g, ch, "%s", rank[i].disp);

    for (d = g->descriptors; d; d = d->next) {
      if (!desc_is_playing(d))
        continue;
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if ((char_get_admlevel(tch) < rank[i].min_level || char_get_admlevel(tch) > rank[i].max_level) && !short_list)
        continue;
      if (!desc_is_playing(d))
        continue;
      if (*name_search && strcasecmp(char_get_name(tch), name_search) &&
          !strstr(char_get_title(tch), name_search))
        continue;
      if (!char_can_see(g, ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) && !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (questwho && !char_prf_flagged(g, tch, PRF_QUEST))
        continue;
      if (localwho && rroom_get_zone(g, char_get_in_room(ch)) != rroom_get_zone(g, char_get_in_room(tch)))
        continue;
      if (who_room && (char_get_in_room(tch) != char_get_in_room(ch)))
        continue;
      if (char_prf_flagged(g, tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT)
        continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
        continue;
      if (showgroup && (!char_get_master(tch) || !char_aff_flagged(tch, AFF_GROUP)))
        continue;
      if (showleader && (!char_get_followers(tch) || !char_aff_flagged(tch, AFF_GROUP)))
        continue;

      if (short_list) {
        send_to_char(g, ch, "               @B[@W%3d @Y%s @C%s@B]@W %-12.12s@n%s@n",
                     char_get_level(tch), char_race_abbr(tch), char_class_abbr(tch), char_get_name(tch),
                     ((!(++num_can_see % 4)) ? "\r\n" : ""));
      } else {
        num_can_see++;

        char usr[100];
        snprintf(usr, sizeof(usr), "@W(@R%s@W)%s", char_get_user(tch), char_plr_flagged(tch, PLR_BIOGR) ? "" : (char_is_spoiled(tch) ? " @R*@n" : ""));
        send_to_char(g, ch, "%s               @D<@C%-12s@D> %s@w%s", line_color, char_get_admlevel(ch) > 0 ? char_get_name(tch) : (char_get_admlevel(tch) > 0 ? char_get_name(tch) : (char_get_user(tch) ? char_get_user(tch) : "NULL")), char_get_admlevel(ch) > 0 ? usr : "", line_color);

        if (char_get_admlevel(tch)) {
          send_to_char(g, ch, " (%s)", admin_level_names[char_get_admlevel(tch)]);
        }

        if (d->snooping && d->snooping->character != ch && char_get_admlevel(ch) >= 3)
          send_to_char(g, ch, " (Snoop: %s)", char_get_name(d->snooping->character));
        if (char_get_invis_level(g, tch))
          send_to_char(g, ch, " (i%d)", char_get_invis_level(g, tch));
        else if (char_aff_flagged(tch, AFF_INVISIBLE))
          send_to_char(g, ch, " (invis)");

        if (char_plr_flagged(tch, PLR_MAILING))
          send_to_char(g, ch, " (mailing)");
        else if (d->olc)
          send_to_char(g, ch, " (OLC)");
        else if (char_plr_flagged(tch, PLR_WRITING))
          send_to_char(g, ch, " (writing)");

        if (d->original)
          send_to_char(g, ch, " (out of body)");

        if (d->connected == CON_OEDIT)
          send_to_char(g, ch, " (O Edit)");
        if (d->connected == CON_MEDIT)
          send_to_char(g, ch, " (M Edit)");
        if (d->connected == CON_ZEDIT)
          send_to_char(g, ch, " (Z Edit)");
        if (d->connected == CON_SEDIT)
          send_to_char(g, ch, " (S Edit)");
        if (d->connected == CON_REDIT)
          send_to_char(g, ch, " (R Edit)");
        if (d->connected == CON_TEDIT)
          send_to_char(g, ch, " (T Edit)");
        if (d->connected == CON_TRIGEDIT)
          send_to_char(g, ch, " (T Edit)");
        if (d->connected == CON_AEDIT)
          send_to_char(g, ch, " (S Edit)");
        if (d->connected == CON_CEDIT)
          send_to_char(g, ch, " (C Edit)");
        if (d->connected == CON_HEDIT)
          send_to_char(g, ch, " (H Edit)");
        if (char_prf_flagged(g, tch, PRF_DEAF))
          send_to_char(g, ch, " (DEAF)");
        if (char_prf_flagged(g, tch, PRF_NOTELL))
          send_to_char(g, ch, " (NO TELL)");
        if (char_prf_flagged(g, tch, PRF_NOGOSS))
          send_to_char(g, ch, " (NO OOC)");
        if (char_plr_flagged(tch, PLR_NOSHOUT))
          send_to_char(g, ch, " (MUTED)");
        if (char_prf_flagged(g, tch, PRF_HIDE))
          send_to_char(g, ch, " (WH)");
        if (char_prf_flagged(g, tch, PRF_BUILDWALK))
          send_to_char(g, ch, " (Buildwalking)");
        if (char_prf_flagged(g, tch, PRF_AFK))
          send_to_char(g, ch, " (AFK)");
        if (char_plr_flagged(tch, PLR_FISHING) && char_get_admlevel(ch) >= ADMLVL_IMMORT)
          send_to_char(g, ch, " (@BFISHING@n)");
        if (char_prf_flagged(g, tch, PRF_NOWIZ))
          send_to_char(g, ch, " (NO WIZ)");
        send_to_char(g, ch, "@n\r\n");
      }
    }
    send_to_char(g, ch, "\r\n");
    if (short_list)
      break;
  }

  if (!num_can_see)
    send_to_char(g, ch, "                            Nobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(g, ch, "                         One lonely character displayed.\r\n");
  else {
    send_to_char(g, ch, "                           @Y%d@w characters displayed.\r\n", num_can_see);
    if (hide > 0) {
      int bam = false;
      if (hide > 1) {
        bam = true;
      }
      send_to_char(g, ch, "                           and @Y%d@w character%s hidden.\r\n", hide, bam ? "s" : "");
    }
  }
  if (g->circle_restrict > 0 && g->circle_restrict <= 100) {
    send_to_char(g, ch, "                      @rThe mud has been wizlocked to lvl %d@n\r\n", g->circle_restrict);
  }
  if (g->circle_restrict == 101) {
    send_to_char(g, ch, "                      @rThe mud has been wizlocked to IMMs only.@n\r\n");
  }
  send_to_char(g, ch, "      @r{@b=================================================================@r}@n\r\n");
  send_to_char(g, ch, "           @cHighest Logon Count Ever@D: @Y%d@w, on %s\r\n", g->HIGHPCOUNT, tmstr);
  send_to_char(g, ch, "                        @cHighest Logon Count Today@D: @Y%d@n\r\n", g->PCOUNT);
}

#define USERS_FORMAT \
  "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users)
{
  char line[200], line2[220], idletime[10];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = CONFIG_LEVEL_CAP, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
  while (*buf) {
    char buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1); /* just in case; we destroy arg in the switch */
      switch (mode) {
        case 'o':
        case 'k':
          outlaws = 1;
          playing = 1;
          strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
          break;
        case 'p':
          playing = 1;
          strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
          break;
        case 'd':
          deadweight = 1;
          strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
          break;
        case 'l':
          playing = 1;
          half_chop(buf1, arg, buf);
          sscanf(arg, "%d-%d", &low, &high);
          break;
        case 'n':
          playing = 1;
          half_chop(buf1, name_search, buf);
          break;
        case 'h':
          playing = 1;
          half_chop(buf1, host_search, buf);
          break;
        default:
          send_to_char(g, ch, "%s", USERS_FORMAT);
          return;
      } /* end of switch */

    } else { /* endif */
      send_to_char(g, ch, "%s", USERS_FORMAT);
      return;
    }
  } /* end while (parser) */
  send_to_char(g, ch,
               "Num Name                 User-name            State          Idl Login    C\r\n"
               "--- -------------------- -------------------- -------------- --- -------- -\r\n");

  one_argument(argument, arg);

  for (d = g->descriptors; d; d = d->next) {
    if (desc_get_connected(d) != CON_PLAYING && playing)
      continue;
    if (desc_get_connected(d) == CON_PLAYING && deadweight)
      continue;
    if (desc_is_playing(d)) {
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if (*host_search && !strstr(d->host, host_search))
        continue;
      if (*name_search && strcasecmp(char_get_name(tch), name_search))
        continue;
      if (!char_can_see(g, ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (char_prf_flagged(g, tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT) {
        continue;
      }
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) &&
          !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
        continue;
      if (showrace && !(showrace & (1U << char_get_race(tch))))
        continue;
      if (char_get_invis_level(g, tch) > char_get_admlevel(ch))
        continue;
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (desc_get_connected(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[desc_get_connected(d)]);

    if (d->character && desc_get_connected(d) == CON_PLAYING && char_get_admlevel(d->character) <= char_get_admlevel(ch))
      snprintf(idletime, sizeof(idletime), "%3d", char_get_timer(d->character) * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    snprintf(line, sizeof(line), "%3d %-20s %-20s %-14s %-3s %-8s %1s ", d->desc_num,
             d->original && char_get_alias(d->original) ? char_get_alias(d->original) : d->character && char_get_alias(d->character) ? char_get_alias(d->character)
                                                                                                       : "UNDEFINED",
             d->user ? d->user : "UNKNOWN", state, idletime, timeptr,
             "N");
    if (*d->host)
      snprintf(line + strlen(line), sizeof(line) - strlen(line), "\n%3d [%s Site: %s]\r\n", d->desc_num, d->user ? d->user : "UNKNOWN", d->host);
    else
      snprintf(line + strlen(line), sizeof(line) - strlen(line), "\n%3d [%s Site: Hostname unknown]\r\n", d->desc_num, d->user ? d->user : "UNKNOWN");

    if (desc_get_connected(d) != CON_PLAYING) {
      snprintf(line2, sizeof(line2), "@g%s@n", line);
      strcpy(line, line2);
    }
    if (desc_get_connected(d) != CON_PLAYING ||
        (desc_get_connected(d) == CON_PLAYING && char_can_see(g, ch, d->character))) {
      send_to_char(g, ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(g, ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}

static void perform_mortal_where(GameWorld *g, struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(g, ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = g->descriptors; d; d = d->next) {
      if (desc_get_connected(d) != CON_PLAYING || d->character == ch)
        continue;
      if ((i = (d->original ? d->original : d->character)) == nullptr)
        continue;
      if (char_get_in_room(i) == NOWHERE || !char_can_see(g, ch, i))
        continue;
      if (rroom_get_zone(g, char_get_in_room(ch)) != rroom_get_zone(g, char_get_in_room(i)))
        continue;
      send_to_char(g, ch, "%-20s - %s\r\n", char_get_name(i), rroom_get_name(g, char_get_in_room(i)));
    }
  } else { /* print only FIRST char, not all. */
    for (i = g->characters; i; i = char_get_next(i)) {
      if (char_get_in_room(i) == NOWHERE || i == ch)
        continue;
      if (!char_can_see(g, ch, i) || rroom_get_zone(g, char_get_in_room(i)) != rroom_get_zone(g, char_get_in_room(ch)))
        continue;
      if (!isname(arg, char_get_alias(i)))
        continue;
      send_to_char(g, ch, "%-25s - %s\r\n", char_get_name(i), rroom_get_name(g, char_get_in_room(i)));
      return;
    }
    send_to_char(g, ch, "Nobody around by that name.\r\n");
  }
}

static void print_object_location(GameWorld *g, int num, struct obj_data *obj, struct char_data *ch,
                                  int recur)
{
  if (num > 0)
    send_to_char(g, ch, "O%3d. %-25s - ", num, obj_get_short_description(obj));
  else
    send_to_char(g, ch, "%33s", " - ");

  if (obj_get_script(obj))
    send_to_char(g, ch, "[T%d]", obj_get_proto_script(obj)->vnum);

  if (obj_get_in_room(obj) != NOWHERE)
    send_to_char(g, ch, "[%5d] %s\r\n", rroom_get_vnum(g, obj_get_in_room(obj)), rroom_get_name(g, obj_get_in_room(obj)));
  else if (obj_get_carried_by(obj))
    send_to_char(g, ch, "carried by %s in room [%d]\r\n", char_pers(g, obj_get_carried_by(obj), ch), rroom_get_vnum(g, char_get_in_room(obj_get_carried_by(obj))));
  else if (obj_get_worn_by(obj))
    send_to_char(g, ch, "worn by %s in room [%d]\r\n", char_pers(g, obj_get_worn_by(obj), ch), rroom_get_vnum(g, char_get_in_room(obj_get_worn_by(obj))));
  else if (obj_get_in_obj(obj)) {
    send_to_char(g, ch, "inside %s%s\r\n", obj_get_short_description(obj_get_in_obj(obj)), (recur ? ", which is" : " "));
    if (recur)
      print_object_location(g, 0, obj_get_in_obj(obj), ch, recur);
  } else
    send_to_char(g, ch, "in an unknown location\r\n");
}

static void perform_immort_where(GameWorld *g, struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, num2 = 0, found = 0;

  const char *planet[11] = {"@GEarth@n",
                            "@CFrigid@n",
                            "@YVegeta@n",
                            "@MKonack@n",
                            "@gNamek@n",
                            "@mAether@n",
                            "@mArlia@n",
                            "@CZenith@n",
                            "@YYardrat@n",
                            "@cKanassa@n",
                            "@RUNKOWN@n"};

  if (!*arg) {
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "GODCMD: %s has checked where to check player locations", char_get_name(ch));
    send_to_char(g, ch, "Players                  Vnum    Planet        Location\r\n-------                 ------   ----------    ----------------\r\n");
    for (d = g->descriptors; d; d = d->next)
      if (desc_is_playing(d)) {
        if (char_get_in_room(d->character) != NOWHERE) {
          if (rroom_flagged(g, char_get_in_room(d->character), ROOM_EARTH)) {
            num2 = 0;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_FRIGID)) {
            num2 = 1;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_VEGETA)) {
            num2 = 2;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_KONACK)) {
            num2 = 3;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_NAMEK)) {
            num2 = 4;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_AETHER)) {
            num2 = 5;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_ARLIA)) {
            num2 = 6;
          } else if (rroom_planet_zenith(g, char_get_in_room(d->character))) {
            num2 = 7;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_YARDRAT)) {
            num2 = 8;
          } else if (rroom_flagged(g, char_get_in_room(d->character), ROOM_KANASSA)) {
            num2 = 9;
          } else {
            num2 = 10;
          }
        }
        i = (d->original ? d->original : d->character);
        if (i && char_can_see(g, ch, i) && (char_get_in_room(i) != NOWHERE)) {
          if (d->original)
            send_to_char(g, ch, "%-20s - [%5d]   %s (in %s)\r\n",
                         char_get_name(i), rroom_get_vnum(g, char_get_in_room(d->character)),
                         rroom_get_name(g, char_get_in_room(d->character)), char_get_name(d->character));
          else {
            send_to_char(g, ch, "%-20s - [%5d]   %-14s %s\r\n", char_get_name(i), rroom_get_vnum(g, char_get_in_room(i)), planet[num2], rroom_get_name(g, char_get_in_room(i)));
          }
        }
      }
  } else {
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "GODCMD: %s has checked where for the location of %s", char_get_name(ch), arg);
    for (i = g->characters; i; i = char_get_next(i)) {
      if (char_can_see(g, ch, i) && char_get_in_room(i) != NOWHERE && isname(arg, char_get_alias(i))) {
        found = 1;
        send_to_char(g, ch, "M%3d. %-25s - [%5d] %-25s", ++num, char_get_name(i),
                     rroom_get_vnum(g, char_get_in_room(i)), rroom_get_name(g, char_get_in_room(i)));
        if (char_is_npc(i) && char_get_script(i)) {
          if (!char_get_script(i)->trig_list->next)
            send_to_char(g, ch, "[T%5d] ", trig_get_vnum(g, char_get_script(i)->trig_list));
          else
            send_to_char(g, ch, "[TRIGS] ");
        }
        send_to_char(g, ch, "\r\n");
      }
    }
    for (k = g->objects; k; k = obj_get_next(k))
      if (char_can_see_obj(g, ch, k) && isname(arg, obj_get_name(k))) {
        found = 1;
        print_object_location(g, ++num, k, ch, true);
      }
    if (!found) {
      send_to_char(g, ch, "Couldn't find any such thing.\r\n");
    } else {
      send_to_char(g, ch, "\r\nFound %d matches.\r\n", num);
    }
  }
}

ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_adm_flagged(ch, ADM_FULLWHERE) || char_get_admlevel(ch) > 4)
    perform_immort_where(g, ch, arg);
  else
    perform_mortal_where(g, ch, arg);
}

ACMD(do_toplist)
{
  if (char_is_npc(ch))
    return;

  FILE *file;
  char fname[40], filler[50], line[256];
  int64_t stats;
  char *title[25] = {nullptr};
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist")) {
    send_to_char(g, ch, "The toplist file does not exist.");
    return;
  }
  if (!(file = fopen(fname, "r"))) {
    send_to_char(g, ch, "The toplist file does not exist.");
    return;
  }
  while (!feof(file) || count < 25) {
    get_line(file, line);
    switch (count) {
      default:
        sscanf(line, "%49s %" SCNd64 "\n", filler, &stats);
        break;
    }
    title[count] = strdup(filler);
    count++;
    *filler = '\0';
  }
  send_to_char(g, ch, "@D-=[@BDBAT Top Lists for @REra@C %d@D]=-@n\r\n", CURRENT_ERA);
  while (x <= count) {
    switch (x) {
      /* Powerlevel Area */
      case 0:
        send_to_char(g, ch, "       @D-@RPowerlevel@D-@n\r\n");
        send_to_char(g, ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 1:
        send_to_char(g, ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 2:
        send_to_char(g, ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 3:
        send_to_char(g, ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 4:
        send_to_char(g, ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      /* Ki Area */
      case 5:
        send_to_char(g, ch, "       @D-@BKi        @D-@n\r\n");
        send_to_char(g, ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 6:
        send_to_char(g, ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 7:
        send_to_char(g, ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 8:
        send_to_char(g, ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 9:
        send_to_char(g, ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      /* Stamina Area */
      case 10:
        send_to_char(g, ch, "       @D-@GStamina   @D-@n\r\n");
        send_to_char(g, ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 11:
        send_to_char(g, ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 12:
        send_to_char(g, ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 13:
        send_to_char(g, ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 14:
        send_to_char(g, ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      /* Stamina Area */
      case 15:
        send_to_char(g, ch, "       @D-@gZenni     @D-@n\r\n");
        send_to_char(g, ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 16:
        send_to_char(g, ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 17:
        send_to_char(g, ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 18:
        send_to_char(g, ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 19:
        send_to_char(g, ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      /* Rpp Area */
      case 20:
        /*send_to_char(ch, "       @D-@mRPP       @D-@n\r\n");
     send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);*/
        free(title[x]);
        break;
      case 21:
        /*send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);*/
        free(title[x]);
        break;
      case 22:
        /*send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);*/
        free(title[x]);
        break;
      case 23:
        /*send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);*/
        free(title[x]);
        break;
      case 24:
        /*send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);*/
        free(title[x]);
        break;
    }
    x++;
  }
  fclose(file);
}

ACMD(do_whois)
{
  char buf[MAX_INPUT_LENGTH];
  int clan                               = false;
  const char *immlevels[ADMLVL_IMPL + 2] = {
      "[Mortal]",               /* lowest admin level */
      "[Enforcer]",             /* lowest admin level +1 */
      "[First Class Enforcer]", /* lowest admin level +2 */
      "[High Enforcer]",        /* lowest admin level +3 */
      "[Vice Admin]",           /* lowest admin level +4 */
      "[Administrator]",        /* lowest admin level +5 */
      "[Implementor]",
  };

  struct char_data *victim = nullptr;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(g, ch, "Who?\r\n");
  } else {
    victim = new char_data();
    clear_char(victim);
    *char_player_specials_slot(victim) = new player_special_data();
    if (load_char(g, argument, victim) >= 0) {
      clan = false;
      send_to_char(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
      if (char_get_admlevel(victim) >= ADMLVL_IMMORT) {
        send_to_char(g, ch, "@cName     @D: @G%s\r\n", char_get_name(victim));
        send_to_char(g, ch, "@cImm Level@D: @G%s\r\n", immlevels[char_get_admlevel(victim)]);
        send_to_char(g, ch, "@cTitle    @D: @G%s\r\n", char_get_title(victim));
      } else {
        send_to_char(g, ch, "@cName  @D: @w%s\r\n@cSensei@D: @w%s\r\n@cRace  @D: @w%s\r\n@cTitle @D: @w%s@n\r\n@cClan  @D: @w%s@n\r\n", char_get_name(victim), char_sensei_name(victim), char_true_race_name(victim), char_get_title(victim), clan ? buf : "None.");
      }
      send_to_char(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
    } else {
      send_to_char(g, ch, "There is no such player.\r\n");
    }
    free_char(g, victim);
  }
}
