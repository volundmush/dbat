/* ************************************************************************
*  File: dg_comm.c                               Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle mud to player communication         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */
#include "dbat/dg_comm.h"
#include "dbat/act.informative.h"
#include "dbat/send.h"
#include "dbat/races.h"
#include "dbat/comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/graph.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/area.h"

/* local functions */
void sub_write_to_char(Character *ch, char *tokens[], void *otokens[], char type[]);


/* same as any_one_arg except that it stops at punctuation */
char *any_one_name(char *argument, char *first_arg) {
    char *arg;

    /* Find first non blank */
    while (isspace(*argument))
        argument++;

    /* Find length of first word */
    for (arg = first_arg;
         *argument && !isspace(*argument) &&
         (!ispunct(*argument) || *argument == '#' || *argument == '-');
         arg++, argument++)
        *arg = LOWER(*argument);
    *arg = '\0';

    return argument;
}


void sub_write_to_char(Character *ch, char *tokens[],
                       void *otokens[], char type[]) {
    char sb[MAX_STRING_LENGTH];
    int i;

    strcpy(sb, "");

    for (i = 0; tokens[i + 1]; i++) {
        strcat(sb, tokens[i]);

        switch (type[i]) {
            case '~':
                if (!otokens[i])
                    strcat(sb, "someone");
                else if ((Character *) otokens[i] == ch)
                    strcat(sb, "you");
                else
                    strcat(sb, PERS((Character *) otokens[i], ch));
                break;

            case '|':
                if (!otokens[i])
                    strcat(sb, "someone's");
                else if ((Character *) otokens[i] == ch)
                    strcat(sb, "your");
                else {
                    strcat(sb, PERS((Character *) otokens[i], ch));
                    strcat(sb, "'s");
                }
                break;

            case '^':
                if (!otokens[i] || !CAN_SEE(ch, (Character *) otokens[i]))
                    strcat(sb, "its");
                else if (otokens[i] == ch)
                    strcat(sb, "your");
                else
                    strcat(sb, HSHR((Character *) otokens[i]));
                break;

            case '&':
                if (!otokens[i] || !CAN_SEE(ch, (Character *) otokens[i]))
                    strcat(sb, "it");
                else if (otokens[i] == ch)
                    strcat(sb, "you");
                else
                    strcat(sb, HSSH((Character *) otokens[i]));
                break;

            case '*':
                if (!otokens[i] || !CAN_SEE(ch, (Character *) otokens[i]))
                    strcat(sb, "it");
                else if (otokens[i] == ch)
                    strcat(sb, "you");
                else
                    strcat(sb, HMHR((Character *) otokens[i]));
                break;

            case '¨':
                if (!otokens[i])
                    strcat(sb, "something");
                else
                    strcat(sb, OBJS(((Object *) otokens[i]), ch));
                break;
        }
    }

    strcat(sb, tokens[i]);
    strcat(sb, "\r\n");
    sb[0] = toupper(sb[0]);
        ch->send_to("%s", sb);
}


void sub_write(char *arg, Character *ch, int8_t find_invis, int targets) {
    char str[MAX_INPUT_LENGTH * 2];
    char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    char *tokens[MAX_INPUT_LENGTH], *s, *p;
    void *otokens[MAX_INPUT_LENGTH];
    Character *to;
    Object *obj;
    int i, tmp;
    int to_sleeping = 1; /* mainly for windows compiles */

    if (!arg)
        return;

    tokens[0] = str;

    for (i = 0, p = arg, s = str; *p;) {
        switch (*p) {
            case '~':
            case '|':
            case '^':
            case '&':
            case '*':
                /* get Character, move to next token */
                type[i] = *p;
                *s = '\0';
                p = any_one_name(++p, name);
                otokens[i] =
                        find_invis ? (void *) get_char_in_room(ch->getRoom(), name) : (void *) get_char_room_vis(
                                ch, name, nullptr);
                tokens[++i] = ++s;
                break;

            case '¨':
                /* get Object, move to next token */
                type[i] = *p;
                *s = '\0';
                p = any_one_name(++p, name);

                if (find_invis) obj = get_obj_in_room(ch->getRoom(), name);
                else if (!(obj = get_obj_in_list_vis(ch, name, nullptr, ch->location.getObjects())));
                else if (!(obj = get_obj_in_equip_vis(ch, name, &tmp, ch->getEquipment())));
                else obj = get_obj_in_list_vis(ch, name, nullptr, ch->getInventory());

                otokens[i] = (void *) obj;
                tokens[++i] = ++s;
                break;

            case '\\':
                p++;
                *s++ = *p++;
                break;

            default:
                *s++ = *p++;
        }
    }

    *s = '\0';
    tokens[++i] = nullptr;

    if (IS_SET(targets, TO_CHAR) && SENDOK(ch))
        sub_write_to_char(ch, tokens, otokens, type);

    if (IS_SET(targets, TO_ROOM)) {
        auto people = ch->location.getPeople();
        for (auto to : filter_raw(people))
            if (to != ch && SENDOK(to))
                sub_write_to_char(to, tokens, otokens, type);
    }
}

void fly_planet(room_vnum roomVnum, const char *messg, Character *ch) {
    if (!messg || !*messg)
        return;

    auto planet = getPlanet(ch->location.getVnum());

    if(!planet) {
        return;
    }

    for(auto i = descriptor_list; i; i = i->next) {
        if(!i->connected) continue;
        if(!i->character) continue;
        if(!AWAKE(i->character)) continue;
        if(IN_ROOM(i->character) == NOWHERE) continue;
        if(!OUTSIDE(i->character)) continue;

        if(planet != getPlanet(i->character->location.getVnum())) continue;

        if (PLR_FLAGGED(i->character, PLR_DISGUISED)) {
            i->send_to("A disguised figure %s", messg);
        } else {
            i->send_to("%s%s %s", readIntro(i->character, ch) == 1 ? "" : "A ",
                       get_i_name(i->character, ch), messg);
        }
    }
}

void fly_zone(Zone *zone, char *messg, Character *ch) {
    if (!messg || !*messg)
        return;

    for (auto i = descriptor_list; i; i = i->next) {
        if (!i->connected && i->character && AWAKE(i->character) && OUTSIDE(i->character) &&
            (i->character->location.getZone() == zone) && i->character != ch) {
            if (PLR_FLAGGED(i->character, PLR_DISGUISED)) {
                i->send_to("A disguised figure %s", messg);
            } else {
                i->send_to("%s%s %s", readIntro(i->character, ch) == 1 ? "" : "A ",
                           get_i_name(i->character, ch), messg);
            }
        }
    }
}

void fly_zone(zone_rnum zone, char *messg, Character *ch) {
    auto &z = zone_table.at(zone);
    fly_zone(&z, messg, ch);
}

void send_to_sense(int type, const char *messg, Character *ch) {
    if (!messg || !*messg)
        return;

    auto planet = getPlanet(ch->location.getVnum());
    if(!planet && type == 0) {
        return;
    }

    for (auto i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING) {
            continue;
        }
        auto tch = i->character;
        if (tch == ch) {
            continue;
        }
        if (ch->location == tch->location)
            continue;
        auto obj = GET_EQ(tch, WEAR_EYE);
        if (!GET_SKILL(tch, SKILL_SENSE)) {
            continue;
        }
        if(auto p = getPlanet(tch->location.getVnum()); type == 0) {
            if (!p) {
                continue;
            }
        }
        if (obj && type == 0) {
            continue;
        }
        if (IS_ANDROID(ch)) {
            continue;
        }

        auto hitch = GET_HIT(ch);
        auto thitch = GET_HIT(tch);
        if (hitch < (thitch * 0.001) + 1)
            continue;

        if (type == 0) {
            auto maxch = GET_MAX_HIT(ch);
            auto maxtch = GET_MAX_HIT(tch);
            if (maxch > maxtch) {
                i->send_to("%s who is stronger than you. They are nearby.\r\n", messg);
            } else if (maxch >= maxtch * .9) {
                i->send_to("%s who is near your strength. They are nearby.\r\n", messg);
            } else if (maxch >= maxtch * .6) {
                i->send_to("%s who is a good bit weaker than you. They are nearby.\r\n", messg);
            } else if (maxch >= maxtch * .4) {
                i->send_to("%s who is a lot weaker than you. They are nearby.\r\n", messg);
            }
            if (readIntro(tch, ch) == 1) {
                i->send_to("@YYou recognise this signal as @y%s@Y!@n\r\n", get_i_name(tch, ch));
            } else if (read_sense_memory(ch, tch)) {
                i->sendText("@YYou recognise this signal, but don't seem to know their name.@n\r\n");
            }
        } else if (planet_check(ch, tch)) {
            char power[MAX_INPUT_LENGTH];
            char align[MAX_INPUT_LENGTH];
            if (hitch > thitch * 10) {
                sprintf(power, ", who is @Runbelievably stronger@Y than you");
            } else if (hitch > thitch * 5) {
                sprintf(power, ", who is much @Rstronger@Y than you");
            } else if (hitch > thitch * 2) {
                sprintf(power, ", who is more than twice as @Rstrong@Y as you");
            } else if (hitch > thitch) {
                sprintf(power, ", who is somewhat @mstronger@Y than you");
            } else if (hitch * 10 < thitch) {
                sprintf(power, ", who is @Munbelievably weaker@Y than you");
            } else if (hitch * 5 < thitch) {
                sprintf(power, ", who is much @Mweaker@Y than you");
            } else if (hitch * 2 < thitch) {
                sprintf(power, ", who is more than twice as @Mweak@Y as you");
            } else if (hitch < thitch) {
                sprintf(power, ", who is somewhat @Wweaker@Y than you");
            } else {
                sprintf(power, ", who is close to @Cequal@Y with you");
            }
            auto al = GET_ALIGNMENT(ch);
            if (al >= 1000) {
                sprintf(align, ", with a @wsaintly@Y aura,");
            } else if (al >= 500) {
                sprintf(align, ", with a very @Cgood@Y aura,");
            } else if (al >= 200) {
                sprintf(align, ", with a @cgood@Y aura,");
            } else if (al > -100) {
                sprintf(align, ", with a near @Wneutral@Y aura,");
            } else if (al > -200) {
                sprintf(align, ", with a sorta @revil@Y aura,");
            } else if (al > -500) {
                sprintf(align, ", with an @revil@Y aura,");
            } else if (al > -900) {
                sprintf(align, ", with a @rvery evil@Y aura,");
            } else {
                sprintf(align, ", with a @rd@De@Wv@wil@Wi@Ds@rh@Y aura,");
            }
            if (strstr(messg, "land"))
                i->send_to("@YYou sense %s%s%s %s! They appear to have landed at...@G%s@n\r\n",
                            readIntro(tch, ch) == 1 ? get_i_name(tch, ch) : "someone", power, align, messg, sense_location(ch));
            else
                i->send_to("@YYou sense %s%s%s %s!@n\r\n",
                            readIntro(tch, ch) == 1 ? get_i_name(tch, ch) : "someone", power, align, messg);
        }
    }
}

void send_to_scouter(const char *messg, Character *ch, int num, int type) {
    if (!messg || !*messg)
        return;

    auto planet = getPlanet(ch->location.getVnum());
    if(!planet && type == 0) {
        return;
    }

    auto pl = ch->getPL();
    auto senseLoc = sense_location(ch);
    
    for (auto i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING) {
            continue;
        }
        auto tch = i->character;
        auto obj = GET_EQ(tch, WEAR_EYE);
        if (tch == ch) continue;
        if(!AWAKE(tch)) continue;

        if(auto p = getPlanet(tch->location.getVnum()); type == 0) {
            if (!p) {
                continue;
            }
        }

        if (GET_INVIS_LEV(ch) > GET_ADMLEVEL(tch)) {
            continue;
        }
        if (IS_ANDROID(ch)) continue;

        if (!obj) continue;
        if (ch->location == tch->location) continue;

        auto scoutVal = obj->getBaseStat<int64_t>(VAL_WORN_SCOUTER);
        if (type == 0) {
            if (num == 1) {
                if (pl >= scoutVal) {
                    i->sendText("@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
                } else {
                    i->send_to("%s@n", messg);
                }
            } else {
                if (pl >= scoutVal) {
                    i->sendText("@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
                } else {
                    i->send_to("%s\r\n", messg);
                }
            }
        } else if (type == 1 && GET_SKILL(tch, SKILL_SENSE) < 20) {
            if (pl >= scoutVal) {
                i->send_to("@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
            } else {
                i->send_to("@D[Blip@D]@w %s. @RPL@D:@Y %s@n\r\n\r\n", messg, add_commas(pl).c_str());
            }
        } else if (type == 2 && GET_SKILL(tch, SKILL_SENSE) < 20) {
            if (pl >= scoutVal) {
                i->send_to("@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, senseLoc);
            } else {
                i->send_to("@D[Blip@D]@w %s at... @G%s. @RPL@D:@Y %s@n\r\n\r\n", messg, senseLoc,
                           add_commas(pl).c_str());
            }
        }
    }
}

void send_to_worlds(Character *ch) {
    char message[MAX_INPUT_LENGTH];

    auto maxh = GET_MAX_HIT(ch);

    if (maxh > 2000000000) {
        sprintf(message, "@RThe whole planet begins to quake violently as if the world is ending!@n\r\n");
    } else if (maxh > 1000000000) {
        sprintf(message, "@RThe whole planet begins to quake violently with a thunderous roar!@n\r\n");
    } else if (maxh > 500000000) {
        sprintf(message, "@RThe whole planet begins to quake violently!@n\r\n");
    } else if (maxh > 100000000) {
        sprintf(message, "@RThe whole planet rumbles and shakes!@n\r\n");
    } else if (maxh > 50000000) {
        sprintf(message, "@RThe whole planet rumbles faintly!@n\r\n");
    } else {
        return;
    }

    auto p = getPlanet(ch->location.getVnum());
    if(!p) return;

    for (auto i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING) {
            continue;
        }
        if(p != getPlanet(i->character->location.getVnum())) continue;
                i->character->send_to("%s", message);
    }
}
