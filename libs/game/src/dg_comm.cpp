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
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/dg_comm.hpp"
#include "dbat/game/act.informative.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/races.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/game/spells.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/planet.hpp"

#include "dbat/game/utils.hpp"
#include "dbat/util/FilterWeak.hpp"

#include "dbat/game/const/Max.hpp"
#include "dbat/game/const/WearSlot.hpp"
#include "dbat/game/const/ItemValues.hpp"

/* local functions */
void sub_write_to_char(Character *ch, char *tokens[], void *otokens[], unsigned char type[]);


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
        *arg = tolower(*argument);
    *arg = '\0';

    return argument;
}


void sub_write_to_char(Character *ch, char *tokens[],
                       void *otokens[], unsigned char type[]) {
    char sb[MAX_STRING_LENGTH];
    int i;
    std::string scratch;

    strcpy(sb, "");

    for (i = 0; tokens[i + 1]; i++) {
        strcat(sb, tokens[i]);

        auto ctoken = static_cast<Character*>(otokens[i]);
        auto itoken = static_cast<Object*>(otokens[i]);

    switch (type[i]) {
            case '~':
                if (!otokens[i])
                    strcat(sb, "someone");
                else if (ctoken == ch)
                    strcat(sb, "you");
                else {
                    scratch = ctoken->displayNameFor(ch);
                    strcat(sb, scratch.c_str());
                }
                break;

            case '|':
                if (!otokens[i])
                    strcat(sb, "someone's");
                else if (ctoken == ch)
                    strcat(sb, "your");
                else {
                    scratch = ctoken->displayNameFor(ch);
                    strcat(sb, scratch.c_str());
                    strcat(sb, "'s");
                }
                break;

            case '^':
                if (!otokens[i] || !ch->canSee(ctoken))
                    strcat(sb, "its");
                else if (ctoken == ch)
                    strcat(sb, "your");
                else
                break;

            case '&':
                if (!ctoken || !ch->canSee(ctoken))
                    strcat(sb, "it");
                else if (ctoken == ch)
                    strcat(sb, "you");
                else
                    strcat(sb, HSSH(ctoken));
                break;

            case '*':
                if (!ctoken || !ch->canSee(ctoken))
                    strcat(sb, "it");
                else if (ctoken == ch)
                    strcat(sb, "you");
                else
                    strcat(sb, HMHR(ctoken));
                break;

            case 0xA8: // '¨' diaeresis as single-byte 0xA8 token
                if (!itoken)
                    strcat(sb, "something");
                else
                    strcat(sb, OBJS(itoken, ch));
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
    unsigned char type[MAX_INPUT_LENGTH];
    char name[MAX_INPUT_LENGTH];
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
    switch ((unsigned char)*p) {
            case '~':
            case '|':
            case '^':
            case '&':
            case '*':
                /* get Character, move to next token */
                type[i] = static_cast<unsigned char>(*p);
                *s = '\0';
                p = any_one_name(++p, name);
                otokens[i] =
                        find_invis ? (void *) get_char_in_room(ch->getRoom(), name) : (void *) get_char_room_vis(
                                ch, name, nullptr);
                tokens[++i] = ++s;
                break;

            case 0xA8: // '¨' diaeresis as single-byte 0xA8 token
                /* get Object, move to next token */
                type[i] = static_cast<unsigned char>(*p);
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
        for (auto to : dbat::util::filter_raw(people))
            if (to != ch && SENDOK(to))
                sub_write_to_char(to, tokens, otokens, type);
    }
}


void Zone::actToOutside(Character *source, const char* messg, bool childrenOnly) {
    auto func = [&](Character *ch) {
        if(ch == source) return;
        if(ch->location == source->location) return;
        if(!AWAKE(ch)) return;
        if(!OUTSIDE(ch)) return;
        act(messg, true, source, nullptr, ch, TO_VICT);
    };

    if(childrenOnly) {
        for(auto z : getChildren()) z->for_each_listening(func);
    } else for_each_listening(func);
}

void Zone::sendToSense(Character *source, const char* messg, bool childrenOnly) {
    auto zchain = source->location.getZone()->getChain();
    // reverse the zchain....
    std::reverse(zchain.begin(), zchain.end());

    auto func = [&](Character* ch) {
        if (ch == source) return;
        if(ch->location == source->location) return;
        if(!AWAKE(ch)) return;
        if (!GET_SKILL(ch, SKILL_SENSE)) return;

        auto alignAura = source->otherSenseAlign(ch);
        auto power = source->otherSensePower(ch);

        std::string name = "someone";
        if(source->isPC) {
            auto recognize = read_sense_memory(source, ch);
            if(recognize) {
                if(readIntro(ch, source) == 1) {
                    name = get_i_name(source, ch);
                }
            } else {
                name = "someone familiar";
            }
        } else {
            name = source->getShortDescription();
        }


        if(boost::icontains(messg, "landing")) {
            auto dest = renderZoneChain(zchain, ch);
            ch->sendFmt("@nYou sense {}@n, who is {} and has {}, {}! They appear to have landed at...{}@n\r\n",
                        name, power, alignAura, messg, dest);
        } else {
            ch->sendFmt("@nYou sense {}@n, who is {} and has {}, {}!@n\r\n",
                        name, power, alignAura, messg);
        }
    };

    if(childrenOnly) {
        for(auto z : getChildren()) z->for_each_listening(func);
    }
    else for_each_listening(func);
}

void send_to_scouter(const char *messg, Character *ch, int num, int type) {
    if (!messg || !*messg)
        return;

    if (IS_ANDROID(ch)) return;

    auto planet = ch->location.getZone()->getRoot();
    if(!planet && type == 0) {
        return;
    }

    auto pl = ch->getPL();

    auto zchain = ch->location.getZone()->getChain();
    std::reverse(zchain.begin(), zchain.end());

    planet->for_each_listening([&](Character *tch) {
        if (tch == ch) return;
        if(tch->location == ch->location) return;
        if (!AWAKE(tch)) return;

        if (GET_INVIS_LEV(ch) > GET_ADMLEVEL(tch)) {
            return;
        }

        auto obj = GET_EQ(tch, WEAR_EYE);
        if (!obj) return;

        if (type == 0 && num == 1) {
            tch->sendText("@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
            return;
        }

        std::string senseLoc = renderZoneChain(zchain, tch);

        auto scoutVal = obj->getBaseStat<int64_t>(VAL_WORN_SCOUTER);
        if (type == 0) {
            if (num == 1) {
                if (pl >= scoutVal) {
                    tch->sendText("@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
                } else {
                    tch->send_to("%s@n", messg);
                }
            } else {
                if (pl >= scoutVal) {
                    tch->sendText("@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
                } else {
                    tch->send_to("%s\r\n", messg);
                }
            }
        } else if (type == 1 && GET_SKILL(tch, SKILL_SENSE) < 20) {
            if (pl >= scoutVal) {
                tch->send_to("@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
            } else {
                tch->send_to("@D[Blip@D]@w %s. @RPL@D:@Y %s@n\r\n\r\n", messg, add_commas(pl).c_str());
            }
        } else if (type == 2 && GET_SKILL(tch, SKILL_SENSE) < 20) {
            if (pl >= scoutVal) {
                tch->send_to("@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, senseLoc);
            } else {
                tch->send_to("@D[Blip@D]@w %s at... @G%s. @RPL@D:@Y %s@n\r\n\r\n", messg, senseLoc,
                           add_commas(pl).c_str());
            }
        }
    });
}

void send_to_worlds(Character *ch) {
    char message[MAX_INPUT_LENGTH];

    auto maxh = ch->getPL();

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

    auto p = ch->location.getZone()->getRoot();
    if(!p) return;

    p->for_each_listening([&](Character *c) {
        if(ch == c) return;
        if(!AWAKE(c)) return;
        c->send_to("%s", message);
    });
}
