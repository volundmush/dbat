/* who.c: who, users, where, finger, whois, toplist
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/time.h"
#include "dbat/help.h"
#include "dbat/command.h"
#include "dbat/flags.h"
#include "dbat/affect.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/races.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/boards.h"
#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"
#include "dbat/disabled.h"
#include "informative_internal.h"

static void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);
static void perform_mortal_where(struct char_data *ch, char *arg);
static void perform_immort_where(struct char_data *ch, char *arg);

ACMD(do_finger)
{

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What user are you wanting to look at?\r\n");
    return;
  }
  if (!readUserIndex(arg)) {
    send_to_char(ch, "That user does not exist\r\n");
    return;
  } else {
    fingerUser(ch, arg);
    return;
  }
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
  char *line_color = "@n";

  skip_spaces(&argument);
  strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  struct {
    char *disp;
    int min_level;
    int max_level;
    int count; /* must always start as 0 */
  } rank[] = {
      {"\r\n               @c------------  @D[    @gI@Gm@Wm@Do@Gr@Dt@Wa@Gl@gs   @D]  @c------------@n\r\n", ADMLVL_IMMORT,     ADMLVL_IMPL,       0},
      {"\r\n@D[@wx@D]@yxxxxxxxxxx@W  [    @GImmortals   @W]  @yxxxxxxxxxx@D[@wx@D]@n\r\n",                  ADMLVL_IMMORT + 8, ADMLVL_GRGOD + 8,  0},
      {"\r\n               @c------------  @D[     @DM@ro@Rr@wt@Ra@rl@Ds    ]  @c------------@n\r\n",       0,                 ADMLVL_IMMORT - 1, 0}
      /*{ "\r\n@GAdministrators@n\r\n\r\n", ADMLVL_GRGOD, ADMLVL_IMPL, 0},
    { "\r\n@GImmortals@n\r\n\r\n"     , ADMLVL_IMMORT, ADMLVL_GRGOD - 1, 0},
    { "\r\n@GMortal@n\r\n\r\n"        , 0, ADMLVL_IMMORT - 1, 0 }*/
  };
  char *tmstr;
  tmstr                        = (char *)asctime(localtime(&PCOUNTDATE));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  int num_ranks = sizeof(rank) / sizeof(rank[0]);
  send_to_char(ch, "\r\n      @r{@b===============  @D[  @DD@wr@ca@Cg@Y(@R*@Y)@Wn@cB@Da@cl@Cl @DA@wd@cv@Ce@Wnt @DT@wr@cu@Ct@Wh@n  @D]  @b===============@r}      @n\r\n");
  for (d = descriptor_list; d && !short_list; d = d->next) {
    if (!desc_is_playing(d))
      continue;
    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (char_get_admlevel(tch) >= ADMLVL_IMMORT)
      line_color = "@w";
    else
      line_color = "@w";

    if (char_can_see(ch, tch) && desc_is_playing(d)) {
      if (*name_search && strcasecmp(char_get_name(tch), name_search) &&
          !strstr(char_get_title(tch), name_search))
        continue;
      if (!char_can_see(ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) && !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (questwho && !char_prf_flagged(tch, PRF_QUEST))
        continue;
      if (localwho && world[char_get_in_room(ch)].zone != world[char_get_in_room(tch)].zone)
        continue;
      if (char_prf_flagged(tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT) {
        hide += 1;
        continue;
      }
      if (who_room && (char_get_in_room(tch) != char_get_in_room(ch)))
        continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
        continue;
      if (showgroup && (!tch->master || !char_aff_flagged(tch, AFF_GROUP)))
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
      send_to_char(ch, "Players\r\n-------\r\n");
    else
      send_to_char(ch, "%s", rank[i].disp);

    for (d = descriptor_list; d; d = d->next) {
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
      if (!char_can_see(ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) && !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (questwho && !char_prf_flagged(tch, PRF_QUEST))
        continue;
      if (localwho && world[char_get_in_room(ch)].zone != world[char_get_in_room(tch)].zone)
        continue;
      if (who_room && (char_get_in_room(tch) != char_get_in_room(ch)))
        continue;
      if (char_prf_flagged(tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT)
        continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
        continue;
      if (showgroup && (!tch->master || !char_aff_flagged(tch, AFF_GROUP)))
        continue;
      if (showleader && (!tch->followers || !char_aff_flagged(tch, AFF_GROUP)))
        continue;

      if (short_list) {
        send_to_char(ch, "               @B[@W%3d @Y%s @C%s@B]@W %-12.12s@n%s@n",
                     char_get_level(tch), RACE_ABBR(tch), CLASS_ABBR(tch), char_get_name(tch),
                     ((!(++num_can_see % 4)) ? "\r\n" : ""));
      } else {
        num_can_see++;

        char usr[100];
        sprintf(usr, "@W(@R%s@W)%s", tch->desc->user, char_plr_flagged(tch, PLR_BIOGR) ? "" : (char_is_spoiled(tch) ? " @R*@n" : ""));
        send_to_char(ch, "%s               @D<@C%-12s@D> %s@w%s", line_color, char_get_admlevel(ch) > 0 ? char_get_name(tch) : (char_get_admlevel(tch) > 0 ? char_get_name(tch) : (char_get_user(tch) ? char_get_user(tch) : "NULL")), char_get_admlevel(ch) > 0 ? usr : "", line_color);

        if (char_get_admlevel(tch)) {
          send_to_char(ch, " (%s)", admin_level_names[char_get_admlevel(tch)]);
        }

        if (d->snooping && d->snooping->character != ch && char_get_admlevel(ch) >= 3)
          send_to_char(ch, " (Snoop: %s)", char_get_name(d->snooping->character));
        if (GET_INVIS_LEV(tch))
          send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
        else if (char_aff_flagged(tch, AFF_INVISIBLE))
          send_to_char(ch, " (invis)");

        if (char_plr_flagged(tch, PLR_MAILING))
          send_to_char(ch, " (mailing)");
        else if (d->olc)
          send_to_char(ch, " (OLC)");
        else if (char_plr_flagged(tch, PLR_WRITING))
          send_to_char(ch, " (writing)");

        if (d->original)
          send_to_char(ch, " (out of body)");

        if (d->connected == CON_OEDIT)
          send_to_char(ch, " (O Edit)");
        if (d->connected == CON_MEDIT)
          send_to_char(ch, " (M Edit)");
        if (d->connected == CON_ZEDIT)
          send_to_char(ch, " (Z Edit)");
        if (d->connected == CON_SEDIT)
          send_to_char(ch, " (S Edit)");
        if (d->connected == CON_REDIT)
          send_to_char(ch, " (R Edit)");
        if (d->connected == CON_TEDIT)
          send_to_char(ch, " (T Edit)");
        if (d->connected == CON_TRIGEDIT)
          send_to_char(ch, " (T Edit)");
        if (d->connected == CON_AEDIT)
          send_to_char(ch, " (S Edit)");
        if (d->connected == CON_CEDIT)
          send_to_char(ch, " (C Edit)");
        if (d->connected == CON_HEDIT)
          send_to_char(ch, " (H Edit)");
        if (char_prf_flagged(tch, PRF_DEAF))
          send_to_char(ch, " (DEAF)");
        if (char_prf_flagged(tch, PRF_NOTELL))
          send_to_char(ch, " (NO TELL)");
        if (char_prf_flagged(tch, PRF_NOGOSS))
          send_to_char(ch, " (NO OOC)");
        if (char_plr_flagged(tch, PLR_NOSHOUT))
          send_to_char(ch, " (MUTED)");
        if (char_prf_flagged(tch, PRF_HIDE))
          send_to_char(ch, " (WH)");
        if (char_prf_flagged(tch, PRF_BUILDWALK))
          send_to_char(ch, " (Buildwalking)");
        if (char_prf_flagged(tch, PRF_AFK))
          send_to_char(ch, " (AFK)");
        if (char_plr_flagged(tch, PLR_FISHING) && char_get_admlevel(ch) >= ADMLVL_IMMORT)
          send_to_char(ch, " (@BFISHING@n)");
        if (char_prf_flagged(tch, PRF_NOWIZ))
          send_to_char(ch, " (NO WIZ)");
        send_to_char(ch, "@n\r\n");
      }
    }
    send_to_char(ch, "\r\n");
    if (short_list)
      break;
  }

  if (!num_can_see)
    send_to_char(ch, "                            Nobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(ch, "                         One lonely character displayed.\r\n");
  else {
    send_to_char(ch, "                           @Y%d@w characters displayed.\r\n", num_can_see);
    if (hide > 0) {
      int bam = FALSE;
      if (hide > 1) {
        bam = TRUE;
      }
      send_to_char(ch, "                           and @Y%d@w character%s hidden.\r\n", hide, bam ? "s" : "");
    }
  }
  if (circle_restrict > 0 && circle_restrict <= 100) {
    send_to_char(ch, "                      @rThe mud has been wizlocked to lvl %d@n\r\n", circle_restrict);
  }
  if (circle_restrict == 101) {
    send_to_char(ch, "                      @rThe mud has been wizlocked to IMMs only.@n\r\n");
  }
  send_to_char(ch, "      @r{@b=================================================================@r}@n\r\n");
  send_to_char(ch, "           @cHighest Logon Count Ever@D: @Y%d@w, on %s\r\n", HIGHPCOUNT, tmstr);
  send_to_char(ch, "                        @cHighest Logon Count Today@D: @Y%d@n\r\n", PCOUNT);
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
          send_to_char(ch, "%s", USERS_FORMAT);
          return;
      } /* end of switch */

    } else { /* endif */
      send_to_char(ch, "%s", USERS_FORMAT);
      return;
    }
  } /* end while (parser) */
  send_to_char(ch,
               "Num Name                 User-name            State          Idl Login    C\r\n"
               "--- -------------------- -------------------- -------------- --- -------- -\r\n");

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
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
      if (!char_can_see(ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (char_prf_flagged(tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT) {
        continue;
      }
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) &&
          !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
        continue;
      if (showrace && !(showrace & (1U << char_get_race(tch))))
        continue;
      if (GET_INVIS_LEV(tch) > char_get_admlevel(ch))
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
      sprintf(idletime, "%3d", d->character->timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    sprintf(line, "%3d %-20s %-20s %-14s %-3s %-8s %1s ", d->desc_num,
            d->original && d->original->name ? d->original->name : d->character && d->character->name ? d->character->name
                                                                                                      : "UNDEFINED",
            d->user ? d->user : "UNKNOWN", state, idletime, timeptr,
            "N");
    if (*d->host)
      sprintf(line + strlen(line), "\n%3d [%s Site: %s]\r\n", d->desc_num, d->user ? d->user : "UNKNOWN", d->host);
    else
      sprintf(line + strlen(line), "\n%3d [%s Site: Hostname unknown]\r\n", d->desc_num, d->user ? d->user : "UNKNOWN");

    if (desc_get_connected(d) != CON_PLAYING) {
      sprintf(line2, "@g%s@n", line);
      strcpy(line, line2);
    }
    if (desc_get_connected(d) != CON_PLAYING ||
        (desc_get_connected(d) == CON_PLAYING && char_can_see(ch, d->character))) {
      send_to_char(ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}

static void perform_mortal_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (desc_get_connected(d) != CON_PLAYING || d->character == ch)
        continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
        continue;
      if (char_get_in_room(i) == NOWHERE || !char_can_see(ch, i))
        continue;
      if (world[char_get_in_room(ch)].zone != world[char_get_in_room(i)].zone)
        continue;
      send_to_char(ch, "%-20s - %s\r\n", char_get_name(i), world[char_get_in_room(i)].name);
    }
  } else { /* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (char_get_in_room(i) == NOWHERE || i == ch)
        continue;
      if (!char_can_see(ch, i) || world[char_get_in_room(i)].zone != world[char_get_in_room(ch)].zone)
        continue;
      if (!isname(arg, i->name))
        continue;
      send_to_char(ch, "%-25s - %s\r\n", char_get_name(i), world[char_get_in_room(i)].name);
      return;
    }
    send_to_char(ch, "Nobody around by that name.\r\n");
  }
}

static void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
                                  int recur)
{
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s - ", num, obj->short_description);
  else
    send_to_char(ch, "%33s", " - ");

  if (SCRIPT(obj))
    send_to_char(ch, "[T%d]", obj->proto_script->vnum);

  if (obj_get_in_room(obj) != NOWHERE)
    send_to_char(ch, "[%5d] %s\r\n", rroom_get_vnum(obj_get_in_room(obj)), world[obj_get_in_room(obj)].name);
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s in room [%d]\r\n", char_pers(obj->carried_by, ch), rroom_get_vnum(char_get_in_room(obj->carried_by)));
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s in room [%d]\r\n", char_pers(obj->worn_by, ch), rroom_get_vnum(char_get_in_room(obj->worn_by)));
  else if (obj->in_obj) {
    send_to_char(ch, "inside %s%s\r\n", obj->in_obj->short_description, (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    send_to_char(ch, "in an unknown location\r\n");
}

static void perform_immort_where(struct char_data *ch, char *arg)
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
    mudlog(LOG_NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "GODCMD: %s has checked where to check player locations", char_get_name(ch));
    send_to_char(ch, "Players                  Vnum    Planet        Location\r\n-------                 ------   ----------    ----------------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (desc_is_playing(d)) {
        if (char_get_in_room(d->character) != NOWHERE) {
          if (rroom_flagged(char_get_in_room(d->character), ROOM_EARTH)) {
            num2 = 0;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_FRIGID)) {
            num2 = 1;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_VEGETA)) {
            num2 = 2;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_KONACK)) {
            num2 = 3;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_NAMEK)) {
            num2 = 4;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_AETHER)) {
            num2 = 5;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_ARLIA)) {
            num2 = 6;
          } else if (rroom_planet_zenith(char_get_in_room(d->character))) {
            num2 = 7;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_YARDRAT)) {
            num2 = 8;
          } else if (rroom_flagged(char_get_in_room(d->character), ROOM_KANASSA)) {
            num2 = 9;
          } else {
            num2 = 10;
          }
        }
        i = (d->original ? d->original : d->character);
        if (i && char_can_see(ch, i) && (char_get_in_room(i) != NOWHERE)) {
          if (d->original)
            send_to_char(ch, "%-20s - [%5d]   %s (in %s)\r\n",
                         char_get_name(i), rroom_get_vnum(char_get_in_room(d->character)),
                         world[char_get_in_room(d->character)].name, char_get_name(d->character));
          else {
            send_to_char(ch, "%-20s - [%5d]   %-14s %s\r\n", char_get_name(i), rroom_get_vnum(char_get_in_room(i)), planet[num2], world[char_get_in_room(i)].name);
          }
        }
      }
  } else {
    mudlog(LOG_NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "GODCMD: %s has checked where for the location of %s", char_get_name(ch), arg);
    for (i = character_list; i; i = i->next) {
      if (char_can_see(ch, i) && char_get_in_room(i) != NOWHERE && isname(arg, i->name)) {
        found = 1;
        send_to_char(ch, "M%3d. %-25s - [%5d] %-25s", ++num, char_get_name(i),
                     rroom_get_vnum(char_get_in_room(i)), world[char_get_in_room(i)].name);
        if (char_is_npc(i) && SCRIPT(i)) {
          if (!TRIGGERS(SCRIPT(i))->next)
            send_to_char(ch, "[T%5d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(i))));
          else
            send_to_char(ch, "[TRIGS] ");
        }
        send_to_char(ch, "\r\n");
      }
    }
    for (k = object_list; k; k = k->next)
      if (char_can_see_obj(ch, k) && isname(arg, k->name)) {
        found = 1;
        print_object_location(++num, k, ch, TRUE);
      }
    if (!found) {
      send_to_char(ch, "Couldn't find any such thing.\r\n");
    } else {
      send_to_char(ch, "\r\nFound %d matches.\r\n", num);
    }
  }
}

ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_adm_flagged(ch, ADM_FULLWHERE) || char_get_admlevel(ch) > 4)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}

ACMD(do_toplist)
{
  if (char_is_npc(ch))
    return;

  FILE *file;
  char fname[40], filler[50], line[256];
  int64_t stats;
  char *title[25] = {""};
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist")) {
    send_to_char(ch, "The toplist file does not exist.");
    return;
  } else if (!(file = fopen(fname, "r"))) {
    send_to_char(ch, "The toplist file does not exist.");
    return;
  }
  while (!feof(file) || count < 25) {
    get_line(file, line);
    switch (count) {
      default:
        sscanf(line, "%s %" I64T "\n", filler, &stats);
        break;
    }
    title[count] = strdup(filler);
    count++;
    *filler = '\0';
  }
  send_to_char(ch, "@D-=[@BDBAT Top Lists for @REra@C %d@D]=-@n\r\n", CURRENT_ERA);
  while (x <= count) {
    switch (x) {
      /* Powerlevel Area */
      case 0:
        send_to_char(ch, "       @D-@RPowerlevel@D-@n\r\n");
        send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 1:
        send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 2:
        send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 3:
        send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 4:
        send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      /* Ki Area */
      case 5:
        send_to_char(ch, "       @D-@BKi        @D-@n\r\n");
        send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 6:
        send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 7:
        send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 8:
        send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 9:
        send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      /* Stamina Area */
      case 10:
        send_to_char(ch, "       @D-@GStamina   @D-@n\r\n");
        send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 11:
        send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 12:
        send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 13:
        send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 14:
        send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      /* Stamina Area */
      case 15:
        send_to_char(ch, "       @D-@gZenni     @D-@n\r\n");
        send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 16:
        send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 17:
        send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 18:
        send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
        free(title[x]);
        break;
      case 19:
        send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
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
  int clan                               = FALSE;
  const char *immlevels[ADMLVL_IMPL + 2] = {
      "[Mortal]",               /* lowest admin level */
      "[Enforcer]",             /* lowest admin level +1 */
      "[First Class Enforcer]", /* lowest admin level +2 */
      "[High Enforcer]",        /* lowest admin level +3 */
      "[Vice Admin]",           /* lowest admin level +4 */
      "[Administrator]",        /* lowest admin level +5 */
      "[Implementor]",
  };

  struct char_data *victim = 0;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Who?\r\n");
  } else {
    CREATE(victim, struct char_data, 1);
    clear_char(victim);
    CREATE(victim->player_specials, struct player_special_data, 1);
    if (load_char(argument, victim) >= 0) {
      clan = FALSE;
      send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
      if (char_get_admlevel(victim) >= ADMLVL_IMMORT) {
        send_to_char(ch, "@cName     @D: @G%s\r\n", char_get_name(victim));
        send_to_char(ch, "@cImm Level@D: @G%s\r\n", immlevels[char_get_admlevel(victim)]);
        send_to_char(ch, "@cTitle    @D: @G%s\r\n", char_get_title(victim));
      } else {
        send_to_char(ch, "@cName  @D: @w%s\r\n@cSensei@D: @w%s\r\n@cRace  @D: @w%s\r\n@cTitle @D: @w%s@n\r\n@cClan  @D: @w%s@n\r\n", char_get_name(victim), SENSEI_NAME(victim), TRUE_RACE(victim), char_get_title(victim), clan ? buf : "None.");
      }
      send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
    } else {
      send_to_char(ch, "There is no such player.\r\n");
    }
    free(victim);
  }
}
