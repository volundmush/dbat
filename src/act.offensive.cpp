/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/act.offensive.h"
#include "dbat/interpreter.h"
#include "dbat/combat.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/utils.h"
#include "dbat/handler.h"
#include "dbat/constants.h"
#include "dbat/fight.h"
#include "dbat/guild.h"
#include "dbat/class.h"
#include "dbat/techniques.h"
#include "dbat/attack.h"

/* Combat commands below this line */

ACMD(do_galikgun) {
    atk::GalikGun a(ch, argument);
    a.execute();
}

ACMD(do_honoo) {
    atk::Honoo a(ch, argument);
    a.execute();
}

ACMD(do_psyblast) {
    atk::PsychicBlast a(ch, argument);
    a.execute();
}

ACMD(do_tslash) {
    atk::TwinSlash a(ch, argument);
    a.execute();
}

ACMD(do_eraser) {
    atk::EraserCannon a(ch, argument);
    a.execute();
}

ACMD(do_pbarrage) {
    atk::PsychicBarrage a(ch, argument);
    a.execute();
}

ACMD(do_geno) {

    int perc, prob;
    double attperc = 0.5, minimum = .4;
    struct char_data *vict = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);
    /* Can they do the technique? */

    if (!can_grav(ch)) {
        return;
    }

    if (!check_skill(ch, SKILL_GENOCIDE)) {
        return;
    }

    if (!limb_ok(ch, 0)) {
        return;
    }

    if (!*arg && !FIGHTING(ch)) {
        send_to_char(ch, "Direct it at who?\r\n");
        return;
    }

    if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

    if (GET_MAX_MANA(ch) * attperc > GET_CHARGE(ch)) {
        attperc = (long double) (GET_CHARGE(ch)) / (long double) (GET_MAX_MANA(ch));
    }

    if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
        return;
    }

    vict = nullptr;
    if (!*arg || !(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch) && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) {
            vict = FIGHTING(ch);
        } else {
            send_to_char(ch, "No one around here by that name.\r\n");
            return;
        }
    }

    if (!can_kill(ch, vict, nullptr, 3)) {
        return;
    }

    if (handle_defender(vict, ch)) {
        struct char_data *def = GET_DEFENDER(vict);
        vict = def;
    }

    prob = init_skill(ch, SKILL_GENOCIDE); /* Set skill value */
    perc = rand_number(1, 115);

    if (prob < perc - 20) {
        act("@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You lose concentration and the ball of energy dissipates!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W loses concentration and the ball of energy dissipates!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        hurt(0, 0, ch, vict, nullptr, 0, 1);
        pcost(ch, attperc, 0);

        improve_skill(ch, SKILL_GENOCIDE, 2);
        return;
    }

    struct obj_data *obj;
    int dista = 15 - (GET_INT(ch) * 0.1);

    if (GET_SKILL(ch, SKILL_GENOCIDE) >= 100) {
        dista -= 3;
    } else if (GET_SKILL(ch, SKILL_GENOCIDE) >= 60) {
        dista -= 2;
    } else if (GET_SKILL(ch, SKILL_GENOCIDE) >= 40) {
        dista -= 1;
    }

    obj = read_object(83, VIRTUAL);
    obj_to_room(obj, IN_ROOM(vict));

    GET_CHARGE(ch) += GET_MAX_HIT(ch) / 10;
    TARGET(obj) = vict;
    KICHARGE(obj) = damtype(ch, 41, prob, attperc);
    KITYPE(obj) = SKILL_GENOCIDE;
    USER(obj) = ch;
    KIDIST(obj) = dista;
    pcost(ch, attperc, 0);
    act("@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You grin viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and you toss it at @c$N@W!@n",
        true, ch, nullptr, vict, TO_CHAR);
    act("@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at YOU!@n",
        true, ch, nullptr, vict, TO_VICT);
    act("@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at @c$N@W!@n",
        true, ch, nullptr, vict, TO_NOTVICT);

    improve_skill(ch, SKILL_GENOCIDE, 2);
}

ACMD(do_genki) {

    int perc, prob;
    double attperc = .5, minimum = .4;
    struct char_data *friend_char = nullptr, *vict = nullptr, *next_v = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);
    /* Can they do the technique? */

    if (!can_grav(ch)) {
        return;
    }

    if (!check_skill(ch, SKILL_GENKIDAMA)) {
        return;
    }

    if (!limb_ok(ch, 0)) {
        return;
    }

    if (!*arg && !FIGHTING(ch)) {
        send_to_char(ch, "Direct it at who?\r\n");
        return;
    }

    if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

    if (GET_MAX_MANA(ch) * attperc > GET_CHARGE(ch)) {
        attperc = (long double) (GET_CHARGE(ch)) / (long double) (GET_MAX_MANA(ch));
    }

    if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
        return;
    }

    vict = nullptr;
    if (!*arg || !(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch) && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) {
            vict = FIGHTING(ch);
        } else {
            send_to_char(ch, "No one around here by that name.\r\n");
            return;
        }
    }

    if (!can_kill(ch, vict, nullptr, 3)) {
        return;
    }

    if (handle_defender(vict, ch)) {
        struct char_data *def = GET_DEFENDER(vict);
        vict = def;
    }

    prob = init_skill(ch, SKILL_GENKIDAMA); /* Set skill value */
    perc = rand_number(1, 115);

    if (prob < perc - 20) {
        act("@WYou raise both your arms upwards and begin to pool your charged ki there. You also start calling on the ki of all living beings in the vicinity who are willing to help. Your concentration wavers though and you waste the energy you have!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. @C$n@W's concentration wavers though and $e wastes the energy $e has!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        hurt(0, 0, ch, vict, nullptr, 0, 1);
        pcost(ch, attperc, 0);
        handle_cooldown(ch, 10);
        improve_skill(ch, SKILL_GENKIDAMA, 2);
        return;
    }

    for (friend_char = ch->getRoom()->people; friend_char; friend_char = next_v) {
        next_v = friend_char->next_in_room;
        if (friend_char == ch) {
            continue;
        }
        if (AFF_FLAGGED(friend_char, AFF_GROUP) &&
            (friend_char->master == ch || ch->master == friend_char || friend_char->master == ch->master)) {
            GET_CHARGE(ch) += (ch->getCurKI()) / 10;
            ch->decCurKI(ch->getCurKI() / 20);
        }
    }

    int dista = 15 - (GET_INT(ch) * 0.1);

    if (GET_SKILL(ch, SKILL_GENKIDAMA) >= 100) {
        dista -= 3;
    } else if (GET_SKILL(ch, SKILL_GENKIDAMA) >= 60) {
        dista -= 2;
    } else if (GET_SKILL(ch, SKILL_GENKIDAMA) >= 40) {
        dista -= 1;
    }

    struct obj_data *obj;

    obj = read_object(82, VIRTUAL);
    obj_to_room(obj, IN_ROOM(vict));

    TARGET(obj) = vict;
    KICHARGE(obj) = damtype(ch, 40, prob, attperc);
    KITYPE(obj) = SKILL_GENKIDAMA;
    USER(obj) = ch;
    KIDIST(obj) = dista;
    pcost(ch, attperc, 0);
    act("@WYou raise both your arms upwards and begin to pool your charged ki there. You also start calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above your hands, when it is finished you lob it toward @c$N@W!@n",
        true, ch, nullptr, vict, TO_CHAR);
    act("@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above $s hands, when it is finished $e lobs it toward YOU!@n",
        true, ch, nullptr, vict, TO_VICT);
    act("@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above $s hands, when it is finished $e lobs it toward @c$N@W!@n",
        true, ch, nullptr, vict, TO_NOTVICT);
    handle_cooldown(ch, 10);
    improve_skill(ch, SKILL_GENKIDAMA, 2);
}

ACMD(do_spiritball) {
    atk::Spiritball a(ch, argument);
    a.execute();
}

ACMD(do_deathball) {
    atk::Deathball a(ch, argument);
    a.execute();
}

ACMD(do_pslash) {
    atk::PhoenixSlash a(ch, argument);
    a.execute();
}

ACMD(do_bigbang) {
    atk::BigBang a(ch, argument);
    a.execute();
}

ACMD(do_scatter) {
    atk::ScatterShot a(ch, argument);
    a.execute();
}

ACMD(do_balefire) {
    atk::Balefire a(ch, argument);
    a.execute();
}
/* Rillao: End Balefire */

ACMD(do_kakusanha) {
    int perc, dge = 2, count = 0, skill;
    int64_t dmg;
    double attperc = .3, minimum = .10;
    struct char_data *vict = nullptr, *next_v = nullptr;
    char arg2[MAX_INPUT_LENGTH];

    one_argument(argument, arg2);

    /* Can they do the technique? */

    if (!can_grav(ch)) {
        return;
    }

    if (!check_skill(ch, SKILL_KAKUSANHA)) {
        return;
    }

    if (!limb_ok(ch, 0)) {
        return;
    }

    if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

    if (GET_MAX_MANA(ch) * attperc > GET_CHARGE(ch)) {
        attperc = (long double) (GET_CHARGE(ch)) / (long double) (GET_MAX_MANA(ch));
    }

    if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
        return;
    }

    /* Passed sanity checks for doing the technique */

    skill = init_skill(ch, SKILL_KAKUSANHA); /* Set skill value */

    for (vict = ch->getRoom()->people; vict; vict = next_v) {
        next_v = vict->next_in_room;
        if (vict == ch) {
            continue;
        }
        if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict)) {
            continue;
        }
        if (GET_HIT(vict) <= 0) {
            continue;
        }
        if (AFF_FLAGGED(vict, AFF_GROUP) && !IS_NPC(vict)) {
            if (vict->master == ch) {
                continue;
            } else if (ch->master == vict) {
                continue;
            } else if (vict->master == ch->master) {
                continue;
            }
        }
        if (MOB_FLAGGED(vict, MOB_NOKILL)) {
            continue;
        }
        if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict)) {
            continue;
        } else {
            count += 1;
        }
    }
    if (count <= 0) {
        send_to_char(ch, "There is no one worth targeting around.\r\n");
        return;
    } /* No one worth targeting */
    else {
        handle_cooldown(ch, 5);
        int hits = 0;
        perc = chance_to_hit(ch);

        if (skill < perc) {
            act("@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. The beam flies off harmlessly through the air as you lose control over it!@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. The beam flies off harmlessly through the air as $e loses control over it!@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            pcost(ch, attperc, 0);

            improve_skill(ch, SKILL_KAKUSANHA, 0);
            return;
        }

        act("@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. Above your targets the beam breaks apart into five seperate pieces that follow their victims!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. Above you the beam breaks apart into five seperate pieces that follow their victims!@n",
            true, ch, nullptr, nullptr, TO_ROOM);

        dmg = damtype(ch, 34, skill, attperc);
        if (count >= 3) {
            dmg = (dmg / 100) * 40;
        } else if (count > 1) {
            dmg = (dmg / 100) * 60;
        }

        for (vict = ch->getRoom()->people; vict; vict = next_v) {
            next_v = vict->next_in_room;
            if (vict == ch) {
                continue;
            }
            if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict)) {
                continue;
            }
            if (AFF_FLAGGED(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict)) {
                continue;
            }
            if (MOB_FLAGGED(vict, MOB_NOKILL)) {
                continue;
            }
            if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict)) {
                continue;
            }
            if (GET_HIT(vict) <= 0) {
                continue;
            }
            if (hits >= 5) {
                continue;
            }
            dge = handle_dodge(vict);
            if (((!IS_NPC(vict) && IS_ICER(vict) && rand_number(1, 30) >= 28) || AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
                (vict->getCurST()) >= 1 && GET_POS(vict) != POS_SLEEPING) {
                hits++;
                act("@C$N@c disappears, avoiding the beam chasing $M!@n", false, ch, nullptr, vict, TO_CHAR);
                act("@cYou disappear, avoiding the beam chasing you!@n", false, ch, nullptr, vict, TO_VICT);
                act("@C$N@c disappears, avoiding the beam chasing $M!@n", false, ch, nullptr, vict, TO_NOTVICT);
                vict->affected_by.reset(AFF_ZANZOKEN);
                pcost(vict, 0, GET_MAX_HIT(vict) / 200);
                hurt(0, 0, ch, vict, nullptr, 0, 1);
                continue;
            } else if (dge + rand_number(-10, 5) > skill) {
                hits++;
                act("@c$N@W manages to escape the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@WYou manage to escape the attack!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$N@W manages to escape the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                improve_skill(vict, SKILL_DODGE, 0);
                hurt(0, 0, ch, vict, nullptr, 0, 1);
                continue;
            } else {
                hits++;
                act("@R$N@r is slammed by one of the beams!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@RYou are slammed by one of the beams!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is slammed by one of the beams!@n", true, ch, nullptr, vict, TO_NOTVICT);
                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                continue;
            }
        } /* Hitting targets! */

        if (count < 5 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE)) {
            send_to_room(IN_ROOM(ch), "The rest of the beams slam into the ground!@n\r\n");
            send_to_room(IN_ROOM(ch), "@wBright explosions erupt from the impacts!\r\n");

            if (SECT(IN_ROOM(ch)) != SECT_INSIDE) {
                impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
                switch (rand_number(1, 8)) {
                    case 1:
                        act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 2:
                        if (rand_number(1, 4) == 4 && ROOM_EFFECT(IN_ROOM(ch)) == 0) {
                            ROOM_EFFECT(IN_ROOM(ch)) = 5;
                            act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!",
                                true, ch, nullptr, nullptr, TO_CHAR);
                            act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!",
                                true, ch, nullptr, nullptr, TO_ROOM);
                        }
                        break;
                    case 3:
                        act("A cloud of dust envelopes the entire area!", true, ch, nullptr, nullptr, TO_CHAR);
                        act("A cloud of dust envelopes the entire area!", true, ch, nullptr, nullptr, TO_ROOM);
                        break;
                    case 4:
                        act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 5:
                        act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 6:
                        act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        break;
                    default:
                        /* we want no message for the default */
                        break;
                }
            }
            if (SECT(IN_ROOM(ch)) == SECT_UNDERWATER) {
                switch (rand_number(1, 3)) {
                    case 1:
                        act("The water churns violently!", true, ch, nullptr, nullptr, TO_CHAR);
                        act("The water churns violently!", true, ch, nullptr, nullptr, TO_ROOM);
                        break;
                    case 2:
                        act("Large bubbles rise from the movement!", true, ch, nullptr, nullptr, TO_CHAR);
                        act("Large bubbles rise from the movement!", true, ch, nullptr, nullptr, TO_ROOM);
                        break;
                    case 3:
                        act("The water collapses in on the hole created!", true, ch, nullptr, nullptr, TO_CHAR);
                        act("The water collapses in on the hole create!", true, ch, nullptr, nullptr, TO_ROOM);
                        break;
                }
            }
            if (SECT(IN_ROOM(ch)) == SECT_WATER_SWIM || SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) {
                switch (rand_number(1, 3)) {
                    case 1:
                        act("A huge column of water erupts from the impact!", true, ch, nullptr, nullptr, TO_CHAR);
                        act("A huge column of water erupts from the impact!", true, ch, nullptr, nullptr, TO_ROOM);
                        break;
                    case 2:
                        act("The impact briefly causes a swirling vortex of water!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("The impact briefly causes a swirling vortex of water!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 3:
                        act("A huge depression forms in the water and erupts into a wave from the impact!", true, ch,
                            nullptr,
                            nullptr, TO_CHAR);
                        act("A huge depression forms in the water and erupts into a wave from the impact!", true, ch,
                            nullptr,
                            nullptr, TO_ROOM);
                        break;
                }
            }
            if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
                impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
                switch (rand_number(1, 8)) {
                    case 1:
                        act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 2:
                        act("The structure of the surrounding room cracks and quakes from the blast!", true, ch,
                            nullptr, nullptr,
                            TO_CHAR);
                        act("The structure of the surrounding room cracks and quakes from the blast!", true, ch,
                            nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 3:
                        act("Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 4:
                        act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 5:
                        act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    case 6:
                        act("The walls of the surrounding room crack in the same instant!", true, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("The walls of the surrounding room crack in the same instant!", true, ch, nullptr, nullptr,
                            TO_ROOM);
                        break;
                    default:
                        /* we want no message for the default */
                        break;
                }
            }

            ch->getRoom()->modDamage((5 - count) * 5);
        }

        pcost(ch, attperc, 0);

        improve_skill(ch, SKILL_KAKUSANHA, 0);
        handle_cooldown(ch, 5);
        return;
    } /* We have targets! Attempt to kills them! */

}

ACMD(do_hellspear) {
    int perc, dge = 2, count = 0, skill;
    int64_t dmg;
    double attperc = .3, minimum = .1;
    struct char_data *vict = nullptr, *next_v = nullptr;
    char arg2[MAX_INPUT_LENGTH];

    one_argument(argument, arg2);

    /* Can they do the technique? */

    if (!can_grav(ch)) {
        return;
    }

    if (!check_skill(ch, SKILL_HELLSPEAR)) {
        return;
    }

    if (!limb_ok(ch, 0)) {
        return;
    }

    if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

    if (GET_MAX_MANA(ch) * attperc > GET_CHARGE(ch)) {
        attperc = (long double) (GET_CHARGE(ch)) / (long double) (GET_MAX_MANA(ch));
    }

    if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
        return;
    }

    /* Passed sanity checks for doing the technique */

    skill = init_skill(ch, SKILL_HELLSPEAR); /* Set skill value */

    for (vict = ch->getRoom()->people; vict; vict = next_v) {
        next_v = vict->next_in_room;
        if (vict == ch) {
            continue;
        }
        if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict)) {
            continue;
        }
        if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict)) {
            continue;
        }
        if (MOB_FLAGGED(vict, MOB_NOKILL)) {
            continue;
        } else {
            count += 1;
        }
    }
    if (count <= 0) {
        send_to_char(ch, "There is no one worth targeting around.\r\n");
        return;
    } /* No one worth targeting */
    else {
        perc = chance_to_hit(ch);

        if (skill < perc) {
            act("@WYou fly up higher in the air while holding your hand up above your head. Your charged ki is condensed and materialized in the grasp of your raised hand forming a spear of energy. The spear disolves as you screw up the technique!@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@W flies up higher in the air while holding $s hand up above $s head. @C$n@W's charged ki is condensed and materialized in the grasp of $s raised hand forming a spear of energy. The spear disolves as $e screws up the technique!@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            pcost(ch, attperc, 0);

            improve_skill(ch, SKILL_HELLSPEAR, 0);
            return;
        }

        act("@WYou fly up higher in the air while holding your hand above your head. Your charged ki is condensed and materialized in the grasp of your raised hand forming a spear of energy. Grinning evily you aim the spear at the ground below and throw it. As the red spear of energy slams into the ground your laughter rings throughout the area, and the @rH@Re@Dl@Wl @rS@Rp@De@Wa@wr B@rl@Ra@Ds@wt@W erupts with a roar!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W flies up higher in the air while holding $s hand above $s head. @C$n@W's charged ki is condensed and materialized in the grasp of $s raised hand forming a spear of energy. Grinning evily $e aims the spear at the ground below and throws it. As the red spear of energy slams into the ground $s laughter rings throughout the area, and the @rH@Re@Dl@Wl @rS@Rp@De@Wa@wr B@rl@Ra@Ds@wt@W erupts with a roar!@n",
            true, ch, nullptr, nullptr, TO_ROOM);

        dmg = damtype(ch, 33, skill, attperc);

        for (vict = ch->getRoom()->people; vict; vict = next_v) {
            next_v = vict->next_in_room;
            if (vict == ch) {
                continue;
            }
            if (MOB_FLAGGED(vict, MOB_NOKILL)) {
                continue;
            }
            if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict)) {
                continue;
            }
            if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict)) {
                continue;
            }
            dge = handle_dodge(vict);

            if (((!IS_NPC(vict) && IS_ICER(vict) && rand_number(1, 30) >= 28) || AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
                (vict->getCurST()) >= 1 && GET_POS(vict) != POS_SLEEPING) {
                act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr,
                    vict,
                    TO_CHAR);
                act("@cYou disappear, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict,
                    TO_VICT);
                act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr,
                    vict,
                    TO_NOTVICT);
                vict->affected_by.reset(AFF_ZANZOKEN);
                pcost(vict, 0, GET_MAX_HIT(vict) / 200);
                continue;
            } else if (dge + rand_number(-10, 5) > skill) {
                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                hurt(0, 0, ch, vict, nullptr, 0, 1);
                improve_skill(vict, SKILL_DODGE, 0);
                continue;
            } else {
                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                if (!AFF_FLAGGED(vict, AFF_FLYING) && GET_POS(vict) == POS_STANDING && rand_number(1, 4) == 4) {
                    handle_knockdown(vict);
                }
                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                continue;
            }
        } /* Hitting targets! */

        pcost(ch, attperc, 0);

        improve_skill(ch, SKILL_HELLSPEAR, 0);
        handle_cooldown(ch, 5);
        return;
    } /* We have targets! Attempt to kills them! */

}

ACMD(do_hellflash) {
    atk::Hellflash a(ch, argument);
    a.execute();
}

ACMD(do_ddslash) {
    atk::DarknessDragonSlash a(ch, argument);
    a.execute();
}

ACMD(do_crusher) {
    atk::CrusherBall a(ch, argument);
    a.execute();
}

ACMD(do_final) {
    atk::FinalFlash a(ch, argument);
    a.execute();
}

ACMD(do_sbc) {
    atk::SpecialBeamCannon a(ch, argument);
    a.execute();
}

ACMD(do_tribeam) {
    atk::Tribeam a(ch, argument);
    a.execute();
}

ACMD(do_kienzan) {
    atk::Kienzan a(ch, argument);
    a.execute();
}

ACMD(do_baku) {
    int perc, dge = 2, count = 0, skill;
    int64_t dmg;
    double attperc = .15, minimum = .05;
    struct char_data *vict = nullptr, *next_v = nullptr;
    char arg2[MAX_INPUT_LENGTH];

    one_argument(argument, arg2);

    /* Can they do the technique? */

    if (!can_grav(ch)) {
        return;
    }

    if (!check_skill(ch, SKILL_BAKUHATSUHA)) {
        return;
    }

    if (!limb_ok(ch, 0)) {
        return;
    }

    if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

    if (GET_MAX_MANA(ch) * attperc > GET_CHARGE(ch)) {
        attperc = (long double) (GET_CHARGE(ch)) / (long double) (GET_MAX_MANA(ch));
    }

    if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
        return;
    }

    /* Passed sanity checks for doing the technique */

    skill = init_skill(ch, SKILL_BAKUHATSUHA); /* Set skill value */

    for (vict = ch->getRoom()->people; vict; vict = next_v) {
        next_v = vict->next_in_room;
        if (vict == ch) {
            continue;
        }
        if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict)) {
            continue;
        }
        if (AFF_FLAGGED(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict)) {
            continue;
        }
        if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict)) {
            continue;
        }
        if (MOB_FLAGGED(vict, MOB_NOKILL)) {
            continue;
        } else {
            count += 1;
        }
    }
    if (count <= 0) {
        send_to_char(ch, "There is no one worth targeting around.\r\n");
        return;
    } /* No one worth targeting */
    else {
        perc = chance_to_hit(ch);
        handle_cooldown(ch, 6);
        if (skill < perc) {
            act("@WYou raise your hand with index and middle fingers extended upwards. You try releasing your charged ki in a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W but mess up and waste the ki!@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@W raises $s hand with index and middle fingers extended upwards. @C$n@W tries releasing $s charged ki in a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W but messes up and wastes the ki!@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            pcost(ch, attperc, 0);

            improve_skill(ch, SKILL_BAKUHATSUHA, 0);
            return;
        }

        act("@WYou raise your hand with index and middle fingers extended upwards. A sudden burst rushes up from the ground as your charged ki explodes in the form of a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W raises $s hand with index and middle fingers extended upwards. A sudden burst rushes up from the ground as $s charged ki explodes in the form of a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W!@n",
            true, ch, nullptr, nullptr, TO_ROOM);

        dmg = damtype(ch, 24, skill, attperc);

        if (GET_SKILL(ch, SKILL_BAKUHATSUHA) >= 100) {
            dmg += GET_MAX_HIT(ch) * 0.08;
        } else if (GET_SKILL(ch, SKILL_BAKUHATSUHA) >= 60) {
            dmg += GET_MAX_HIT(ch) * 0.04;
        } else if (GET_SKILL(ch, SKILL_BAKUHATSUHA) >= 40) {
            dmg += GET_MAX_HIT(ch) * 0.02;
        }

        switch (count) {
            case 1:
                dmg = dmg;
                break;
            case 2:
                dmg = (dmg / 100) * 75;
                break;
            case 3:
                dmg = (dmg / 100) * 50;
                break;
            default:
                dmg = (dmg / 100) * 25;
                break;
        }
        for (vict = ch->getRoom()->people; vict; vict = next_v) {
            next_v = vict->next_in_room;
            if (vict == ch) {
                continue;
            }
            if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict)) {
                continue;
            }
            if (AFF_FLAGGED(vict, AFF_GROUP) &&
                (vict->master == ch || ch->master == vict || vict->master == ch->master)) {
                continue;
            }
            if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict)) {
                continue;
            }
            if (MOB_FLAGGED(vict, MOB_NOKILL)) {
                continue;
            }
            dge = handle_dodge(vict);
            if (((!IS_NPC(vict) && IS_ICER(vict) && rand_number(1, 30) >= 28) || AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
                (vict->getCurST()) >= 1 && GET_POS(vict) != POS_SLEEPING) {
                act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr,
                    vict,
                    TO_CHAR);
                act("@cYou disappear, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict,
                    TO_VICT);
                act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr,
                    vict,
                    TO_NOTVICT);
                vict->affected_by.reset(AFF_ZANZOKEN);
                pcost(vict, 0, GET_MAX_HIT(vict) / 200);
                hurt(0, 0, ch, vict, nullptr, 0, 1);
                continue;
            } else if (dge + rand_number(-15, 5) > skill) {
                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                hurt(0, 0, ch, vict, nullptr, 0, 1);
                improve_skill(vict, SKILL_DODGE, 0);
                continue;
            } else {
                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                if (!AFF_FLAGGED(vict, AFF_FLYING) && GET_POS(vict) == POS_STANDING) {
                    handle_knockdown(vict);
                }
                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                continue;
            }
        } /* Hitting targets! */

        pcost(ch, attperc, 0);

        improve_skill(ch, SKILL_BAKUHATSUHA, 0);
        return;
    } /* We have targets! Attempt to kills them! */

}

ACMD(do_rogafufuken) {
    atk::Rogafufuken a(ch, argument);
    a.execute();
}

ACMD(do_dualbeam) {
    atk::DualBeam a(ch, argument);
    a.execute();
}

/* Chimera */
ACMD(do_blessedhammer) {
    int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
    int64_t dmg;
    double attperc = .05, minimum = .01;
    struct char_data *vict;
    struct obj_data *obj;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    /* Can they do the technique? */

    if (!can_grav(ch)) {
        return;
    }

    if (!check_skill(ch, SKILL_BLESSEDHAMMER)) {
        return;
    }

    if (!*arg && !FIGHTING(ch)) {
        send_to_char(ch, "Direct it at who?\r\n");
        return;
    }

    if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

    if (GET_MAX_MANA(ch) * attperc > GET_CHARGE(ch)) {
        attperc = (long double) (GET_CHARGE(ch)) / (long double) (GET_MAX_MANA(ch));
    }

    if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
        return;
    }

    /* Passed sanity checks for doing the technique */

    skill = init_skill(ch, SKILL_BLESSEDHAMMER); /* Set skill value */

    if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

    /* There is a player/mob targeted */
    handle_cooldown(ch, 5);
    if (vict) {
        if (!can_kill(ch, vict, nullptr, 1)) {
            return;
        }
        if (handle_defender(vict, ch)) {
            struct char_data *def = GET_DEFENDER(vict);
            vict = def;
        }
        improve_skill(ch, SKILL_BLESSEDHAMMER, 0);

        index = check_def(vict); /* Check parry/block/dodge of vict */

        prob = roll_accuracy(ch, skill, true);
        perc = chance_to_hit(ch);

        index -= handle_speed(ch, vict);

        avo = index / 4;

        handle_defense(vict, &pry, &blk, &dge);

        prob += 15; /* kousengan bonus */

        prob -= avo;
        tech_handle_posmodifier(vict, pry, blk, dge, prob);

        if (!tech_handle_zanzoken(ch, vict, "@WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@C")) {
            pcost(ch, attperc, 0);
            pcost(vict, 0, GET_MAX_HIT(vict) / 200);
            return;
        }

        if (prob < perc - 20) {
            if ((vict->getCurST()) > 0) {
                if (blk > axion_dice(10)) {
                    act("@C$N@W moves quickly and blocks your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", false, ch,
                        nullptr, vict, TO_CHAR);
                    act("@WYou move quickly and block @C$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", false,
                        ch, nullptr, vict, TO_VICT);
                    act("@C$N@W moves quickly and blocks @c$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", false,
                        ch, nullptr, vict, TO_NOTVICT);
                    pcost(ch, attperc, 0);
                    pcost(vict, 0, GET_MAX_HIT(vict) / 500);
                    dmg = damtype(ch, 42, skill, attperc);
                    dmg /= 4;
                    hurt(0, 0, ch, vict, nullptr, dmg, 1);

                    return;
                } else if (dge > axion_dice(10)) {
                    act("@C$N@W manages to dodge your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n",
                        false, ch, nullptr, vict, TO_CHAR);
                    act("@WYou dodge @C$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n",
                        false, ch, nullptr, vict, TO_VICT);
                    act("@C$N@W manages to dodge @c$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n",
                        false, ch, nullptr, vict, TO_NOTVICT);
                    send_to_room(IN_ROOM(vict), "@wA bright explosion erupts from the impact!\r\n");

                    dodge_ki(ch, vict, 0, 17, skill, SKILL_BLESSEDHAMMER); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

                    ch->getRoom()->modDamage(5);
                    improve_skill(vict, SKILL_DODGE, 0);

                    pcost(ch, attperc, 0);
                    hurt(0, 0, ch, vict, nullptr, 0, 1);

                    return;
                } else {
                    act("@WYou can't believe it but your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W misses, flying through the air harmlessly!@n",
                        false, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at you, but misses!@n ", false, ch,
                        nullptr, vict, TO_VICT);
                    act("@c$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at @C$N@W, but somehow misses!@n ",
                        false, ch, nullptr, vict, TO_NOTVICT);

                    pcost(ch, attperc, 0);
                    hurt(0, 0, ch, vict, nullptr, 0, 1);

                    return;
                }
            } else {
                act("@WYou can't believe it but your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W misses, flying through the air harmlessly!@n",
                    false, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at you, but misses!@n", false, ch,
                    nullptr,
                    vict, TO_VICT);
                act("@c$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at @C$N@W, but somehow misses!@n", false,
                    ch, nullptr, vict, TO_NOTVICT);
                pcost(ch, attperc, 0);

            }
            hurt(0, 0, ch, vict, nullptr, 0, 1);
            return;
        } else {
            dmg = damtype(ch, 42, skill, attperc);
            if (AFF_FLAGGED(vict, AFF_SANCTUARY)) {
                dmg *= calc_critical(ch, 1);
            }
            int hitspot = 1;
            hitspot = roll_hitloc(ch, vict, skill);
            switch (hitspot) {
                case 1:
                    act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their chest!@n",
                        true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR chest!@n",
                        true, ch, nullptr, vict, TO_VICT);
                    act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's chest!@n",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    if (GET_BONUS(ch, BONUS_SOFT)) {
                        dmg *= calc_critical(ch, 2);
                    }
                    hurt(0, 0, ch, vict, nullptr, dmg, 1);
                    dam_eq_loc(vict, 4);
                    /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                    break;
                case 2: /* Critical */
                    if (!GET_BONUS(ch, BONUS_POWERHIT)) {
                        dmg *= 3;
                    } else {
                        dmg *= 5;
                    }
                    act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their face!@n",
                        true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR face!@n",
                        true, ch, nullptr, vict, TO_VICT);
                    act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's face!@n",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    hurt(0, 0, ch, vict, nullptr, dmg, 1);
                    dam_eq_loc(vict, 3);
                    /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                    break;
                case 3:
                    act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their gut!@n",
                        true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR gut!@n",
                        true, ch, nullptr, vict, TO_VICT);
                    act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's gut!@n",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    if (GET_BONUS(ch, BONUS_SOFT)) {
                        dmg *= calc_critical(ch, 2);
                    }
                    hurt(0, 0, ch, vict, nullptr, dmg, 1);
                    dam_eq_loc(vict, 4);
                    /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                    break;
                case 4: /* Weak */
                    act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their arm!@n",
                        true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR arm!@n",
                        true, ch, nullptr, vict, TO_VICT);
                    act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's arm!@n",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    dmg *= calc_critical(ch, 1);
                    hurt(0, 190, ch, vict, nullptr, dmg, 1);
                    dam_eq_loc(vict, 1);
                    /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                    break;
                case 5: /* Weak 2 */
                    act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their leg!@n",
                        true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR leg!@n",
                        true, ch, nullptr, vict, TO_VICT);
                    act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's leg!@n",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    dmg *= calc_critical(ch, 1);
                    hurt(1, 190, ch, vict, nullptr, dmg, 1);
                    dam_eq_loc(vict, 2);
                    /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                    break;
            }
            if (!AFF_FLAGGED(vict, AFF_BURNED) && rand_number(1, 4) == 3 && !IS_DEMON(vict)) {
                send_to_char(vict, "@RYou are burned by the attack!@n\r\n");
                send_to_char(ch, "@RThey are burned by the attack!@n\r\n");
                vict->affected_by.set(AFF_BURNED);
            }
            pcost(ch, attperc, 0);

            return;
        }
    } else if (obj) {
        if (!can_kill(ch, nullptr, obj, 1)) {
            return;
        }
        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "It is broken already!\r\n");
            return;
        }
        dmg = damtype(ch, 42, skill, attperc);
        dmg /= 10;
        act("@WYou fire a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
        act("@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
        hurt(0, 0, ch, nullptr, obj, dmg, 0);
        pcost(ch, attperc, 0);

    } else {
        send_to_char(ch, "Error! Please report.\r\n");
        return;
    }
}

ACMD(do_kousengan) {
    atk::Kousengan a(ch, argument);
    a.execute();
}

ACMD(do_deathbeam) {
    atk::DeathBeam a(ch, argument);
    a.execute();
}

ACMD(do_dodonpa) {
    atk::Dodonpa a(ch, argument);
    a.execute();
}

ACMD(do_masenko) {
    atk::Masenko a(ch, argument);
    a.execute();
}

ACMD(do_kamehameha) {
    atk::Kamehameha a(ch, argument);
    a.execute();
}

ACMD(do_renzo) {
    atk::Renzo a(ch, argument);
    a.execute();
}

ACMD(do_heeldrop) {
    atk::Heeldrop a(ch, argument);
    a.execute();
}

ACMD(do_attack) {
    int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0, wtype = 0, gun = false, gun2 = false;
    int dualwield = 0, wielded = 0, guncost = 0;
    int64_t stcost = (GET_MAX_HIT(ch) / 150);
    int64_t dmg;
    struct char_data *vict;
    struct obj_data *obj = nullptr;
    char arg[MAX_INPUT_LENGTH];
    double attperc = 0;

    if (IS_ANDROID(ch)) {
        stcost *= 0.25;
    }
    one_argument(argument, arg);

    if (!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) {
        send_to_char(ch,
                     "You need to wield a weapon to use this, without one try punch, kick, or other no weapon attacks.\r\n");
        return;
    }

    if (!can_grav(ch)) {
        return;
    }

    if (!*arg && !FIGHTING(ch)) {
        send_to_char(ch, "Direct it at who?\r\n");
        return;
    }
    if (GET_EQ(ch, WEAR_WIELD1)) {
        if (!IS_ANDROID(ch)) {
            stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD1));
        } else {
            stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD1)) * 0.25;
        }
        if (!check_points(ch, 0, stcost)) {
            return;
        }
        wielded = 1;
    } else if (GET_EQ(ch, WEAR_WIELD2)) {
        if (!IS_ANDROID(ch)) {
            stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2));
        } else {
            stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2)) * 0.25;
        }
        if (!check_points(ch, 0, stcost)) {
            return;
        }
    }

    if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

    if (GET_EQ(ch, WEAR_WIELD1)) {
        if (vict) {
            if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
                if (!can_kill(ch, vict, nullptr, 1)) {
                    return;
                }
            } else {
                if (!can_kill(ch, vict, nullptr, 0)) {
                    return;
                }
            }
        }
        if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
            skill = init_skill(ch, SKILL_DAGGER);
            improve_skill(ch, SKILL_DAGGER, 1);
            wtype = 1;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
            skill = init_skill(ch, SKILL_SWORD);
            improve_skill(ch, SKILL_SWORD, 1);
            wtype = 0;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
            skill = init_skill(ch, SKILL_CLUB);
            improve_skill(ch, SKILL_CLUB, 1);
            wtype = 2;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
            skill = init_skill(ch, SKILL_SPEAR);
            improve_skill(ch, SKILL_SPEAR, 1);
            wtype = 3;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
            gun = true;
            skill = init_skill(ch, SKILL_GUN);
            improve_skill(ch, SKILL_GUN, 1);
            wtype = 4;
        } else {
            skill = init_skill(ch, SKILL_BRAWL);
            improve_skill(ch, SKILL_BRAWL, 1);
            wtype = 5;
        }
    } else if (GET_EQ(ch, WEAR_WIELD2)) {
        if (vict) {
            if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
                if (!can_kill(ch, vict, nullptr, 1)) {
                    return;
                }
            } else {
                if (!can_kill(ch, vict, nullptr, 0)) {
                    return;
                }
            }
        }
        if (wielded == 1)
            wielded = 2;
        if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
            skill = init_skill(ch, SKILL_DAGGER);
            improve_skill(ch, SKILL_DAGGER, 1);
            wtype = 1;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
            skill = init_skill(ch, SKILL_SWORD);
            improve_skill(ch, SKILL_SWORD, 1);
            wtype = 0;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
            skill = init_skill(ch, SKILL_CLUB);
            improve_skill(ch, SKILL_CLUB, 1);
            wtype = 2;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
            skill = init_skill(ch, SKILL_SPEAR);
            improve_skill(ch, SKILL_SPEAR, 1);
            wtype = 3;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
            gun2 = true;
            skill = init_skill(ch, SKILL_GUN);
            improve_skill(ch, SKILL_GUN, 1);
            wtype = 4;
        } else {
            skill = init_skill(ch, SKILL_BRAWL);
            improve_skill(ch, SKILL_BRAWL, 1);
            wtype = 5;
        }
    }
    if (wielded == 2 && gun == false) {
        if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 100) {
            dualwield = 3;
            stcost -= stcost * 0.30;
        } else if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 75) {
            dualwield = 2;
            stcost -= stcost * 0.25;
        }
    }

    int wlvl = 0;
    struct obj_data *weap = nullptr;
    if (GET_EQ(ch, WEAR_WIELD1)) {
        weap = GET_EQ(ch, WEAR_WIELD1);
    } else {
        weap = GET_EQ(ch, WEAR_WIELD2);
    }
    if (OBJ_FLAGGED(weap, ITEM_WEAPLVL1)) {
        wlvl = 1;
    } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL2)) {
        wlvl = 2;
    } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL3)) {
        wlvl = 3;
    } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL4)) {
        wlvl = 4;
    } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL5)) {
        wlvl = 5;
    }

    if (GET_PREFERENCE(ch) != PREFERENCE_H2H) {
        handle_cooldown(ch, 4);
    } else {
        handle_cooldown(ch, 8);
    }

    if (wielded == 1 && (gun == true || gun2 == true)) {
        if (wlvl == 5) {
            guncost = 12;
        } else if (wlvl == 4) {
            guncost = 6;
        } else if (wlvl == 3) {
            guncost = 4;
        } else if (wlvl == 2) {
            guncost = 2;
        }
        if (GET_GOLD(ch) < guncost) {
            send_to_char(ch, "You do not have enough zenni. You need %d zenni per shot for that level of gun.\r\n",
                         guncost);
            return;
        } else {
            ch->mod(CharMoney::Carried, -guncost);
        }
    } else if (wielded == 2 && gun == true) {
        if (wlvl == 5) {
            guncost = 12;
        } else if (wlvl == 4) {
            guncost = 6;
        } else if (wlvl == 3) {
            guncost = 4;
        } else if (wlvl == 2) {
            guncost = 2;
        }
        if (GET_GOLD(ch) < guncost) {
            send_to_char(ch, "You do not have enough zenni. You need %d zenni per shot for that level of gun.\r\n",
                         guncost);
            return;
        } else {
            ch->mod(CharMoney::Carried, -guncost);
        }
    }

    if (vict) {
        if (handle_defender(vict, ch)) {
            struct char_data *def = GET_DEFENDER(vict);
            vict = def;
        }

        index = check_def(vict);
        prob = roll_accuracy(ch, skill, false);
        perc = chance_to_hit(ch);

        index -= handle_speed(ch, vict);

        avo = index / 4;
        handle_defense(vict, &pry, &blk, &dge);

        if (gun == true) {
            if (dualwield >= 2) {
                prob += prob * 0.1;
            }
        }

        prob -= avo;
        if (PLR_FLAGGED(ch, PLR_THANDW)) {
            perc += 15;
        }
        tech_handle_posmodifier(vict, pry, blk, dge, prob);

        if (!tech_handle_zanzoken(ch, vict, "attack")) {
            if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
                pcost(ch, 0, stcost / 3);
            pcost(vict, 0, GET_MAX_HIT(vict) / 150);
            return;
        }

        if (prob < perc - 20) {
            if ((vict->getCurST()) > 0) {
                if (pry > rand_number(1, 140) && (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY))) {
                    act("@C$N@W intercepts and parries your attack with $S own!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@WYou intercept and parry @C$n's@W attack with one of your own!@n", true, ch, nullptr, vict,
                        TO_VICT);
                    act("@C$N@W intercepts and parries @c$n's@W attack with one of $S own!@n", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    if (wtype != 4)
                        handle_disarm(ch, vict);
                    improve_skill(vict, SKILL_PARRY, 0);
                    if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
                        pcost(ch, 0, stcost);
                    pcost(vict, 0, GET_MAX_HIT(vict) / 500);
                    dmg = damtype(vict, -2, skill, attperc);
                    dmg *= calc_critical(ch, 1);
                    hurt(0, 0, vict, ch, nullptr, dmg, -1);

                    return;
                } else if (blk > axion_dice(10)) {
                    act("@C$N@W moves quickly and blocks your attack!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@WYou move quickly and block @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@C$N@W moves quickly and blocks @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    improve_skill(vict, SKILL_BLOCK, 0);
                    if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
                        pcost(ch, 0, stcost);
                    pcost(vict, 0, GET_MAX_HIT(vict) / 500);
                    dmg = damtype(ch, -1, skill, attperc);
                    dmg /= 4;
                    hurt(0, 0, ch, vict, nullptr, dmg, 0);

                    return;
                } else if (dge > axion_dice(10)) {
                    act("@C$N@W manages to dodge your attack!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@WYou dodge @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@C$N@W manages to dodge @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    improve_skill(vict, SKILL_DODGE, 0);
                    if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
                        pcost(ch, 0, stcost);
                    hurt(0, 0, ch, vict, nullptr, 0, 0);

                    return;
                } else {
                    act("@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
                        pcost(ch, 0, stcost / 3);
                    hurt(0, 0, ch, vict, nullptr, 0, 0);

                    return;
                }
            } else {
                act("@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
                if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
                    pcost(ch, 0, stcost / 3);

            }
            hurt(0, 0, ch, vict, nullptr, 0, 0);
            return;
        } else {
            dmg = damtype(ch, -1, skill, attperc);
            if (OBJ_FLAGGED(weap, ITEM_WEAPLVL1)) {
                dmg += dmg * 0.05;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL2)) {
                dmg += dmg * 0.1;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL3)) {
                dmg += dmg * 0.2;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL4)) {
                dmg += dmg * 0.3;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL5)) {
                dmg += dmg * 0.5;
            }
            if (wtype == 5) {
                if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
                    dmg += dmg * 0.5;
                    wlvl = 5;
                } else if (GET_SKILL(ch, SKILL_BRAWL) >= 50) {
                    dmg += dmg * 0.2;
                    wlvl = 3;
                }
            }
            if (wtype == 0 && IS_KONATSU(ch)) {
                dmg += dmg * .25;
            }
            if (PLR_FLAGGED(ch, PLR_THANDW)) {
                dmg += dmg * 1.2;
            }
            if (!IS_NPC(ch)) {
                if (PLR_FLAGGED(ch, PLR_THANDW) && gun == false && gun2 == false) {
                    if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 100) {
                        dmg += dmg * 0.5;
                    } else if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 75) {
                        dmg += dmg * 0.25;
                    } else if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 50) {
                        dmg += dmg * 0.1;
                    }
                    if (wtype == 3) {
                        switch (wlvl) {
                            case 1:
                                dmg += dmg * 0.04;
                                break;
                            case 2:
                                dmg += dmg * 0.08;
                                break;
                            case 3:
                                dmg += dmg * 0.12;
                                break;
                            case 4:
                                dmg += dmg * 0.2;
                                break;
                            case 5:
                                dmg += dmg * 0.25;
                                break;
                        }
                    }
                }
            }
            if (wtype == 3) {
                if (skill >= 100)
                    dmg += dmg * 0.04;
                else if (skill >= 50)
                    dmg += dmg * 0.1;
            }
            int hitspot = 1;
            if (gun == true)
                dmg = gun_dam(ch, wlvl);
            hitspot = roll_hitloc(ch, vict, skill);
            int64_t beforepl = GET_HIT(vict);
            switch (hitspot) {
                case 1:
                    if (GET_BONUS(ch, BONUS_SOFT)) {
                        dmg *= calc_critical(ch, 2);
                    }
                    break;
                case 2: /* Head */
                    hitspot = 4;
                    break;
                case 3: /* Body */
                    hitspot = 5;
                    if (GET_BONUS(ch, BONUS_SOFT)) {
                        dmg *= calc_critical(ch, 2);
                    }
                    break;
                case 4: /* Arm */
                    hitspot = 2;
                    break;
                case 5: /* Leg */
                    hitspot = 3;
                    break;
            }
            if (PLR_FLAGGED(ch, PLR_THANDW) && gun == true) {
                if (hitspot != 4 && boom_headshot(ch)) {
                    hitspot = 4;
                    send_to_char(ch, "@GBoom headshot!@n\r\n");
                }
            }
            switch (wtype) {
                case 0:
                    switch (hitspot) {
                        case 1:
                            act("@WYou slash @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou slash @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou slash @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou slash @C$N@W across the face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            if (!IS_NPC(ch)) {
                                if (PLR_FLAGGED(ch, PLR_THANDW) && gun == false && gun2 == false) {
                                    if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 100) {
                                        double mult = calc_critical(ch, 0);
                                        mult += 1.0;
                                        dmg *= mult;
                                    } else {
                                        dmg *= calc_critical(ch, 0);
                                    }
                                } else {
                                    dmg *= calc_critical(ch, 0);
                                }
                            } else {
                                dmg *= calc_critical(ch, 0);
                            }
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou slash @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end slash switch*/
                    if (beforepl - GET_HIT(vict) >= (vict->getEffMaxPL()) * 0.025) {
                        cut_limb(ch, vict, wlvl, hitspot);
                    }
                    break;
                case 1:
                    if (!FIGHTING(ch) && backstab(ch, vict, wlvl, dmg)) {
                        if (vict != nullptr && GET_HIT(vict) > 1 && axion_dice(0) < (GET_SKILL(ch, SKILL_DUALWIELD)) &&
                            GET_EQ(ch, WEAR_WIELD1) && GET_EQ(ch, WEAR_WIELD2)) {
                            do_attack2(ch, nullptr, 0, 0);
                        }
                        pcost(ch, 0, stcost);
                        return;
                    }
                    dmg += (dmg * 0.01) * (GET_DEX(ch) * 0.5);
                    switch (hitspot) {
                        case 1:
                            act("@WYou pierce @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou pierce @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou pierce @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou pierce @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou pierce @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end pierce switch*/
                    break;
                case 2:
                    switch (hitspot) {
                        case 1:
                            act("@WYou crush @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W chest@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou crush @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou crush @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou crush @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes you in the face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou crush @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end crush switch*/
                    club_stamina(ch, vict, wlvl, dmg);
                    break;
                case 3:
                    switch (hitspot) {
                        case 1:
                            act("@WYou stab @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou stab @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou stab @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou stab @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs you in the face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou stab @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end stab switch*/
                    break;
                case 4:
                    switch (hitspot) {
                        case 1:
                            act("@WYou blast @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou blast @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou blast @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou blast @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts you in the face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou blast @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end blast switch*/
                    break;
                case 5:
                    switch (hitspot) {
                        case 1:
                            act("@WYou whack @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou whack @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou whack @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou whack @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks you in the face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
                                double mult = calc_critical(ch, 0);
                                mult += 1.0;
                                dmg *= mult;
                            } else {
                                dmg *= calc_critical(ch, 0);
                            }
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou whack @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end brawl switch*/
                    break;
            }/* end switch one*/
        }
        if ((wielded == 2 && gun == false) || (gun2 == false && gun == false)) {
            if (GET_EQ(ch, WEAR_WIELD1)) {
                if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) && AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
                    !GET_BONUS(ch, BONUS_FIREPROOF) && !IS_DEMON(ch)) {
                    act("@c$N's@W fireshield burns your weapon!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n's@W weapon is burned by your fireshield!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    int damdam = GET_SKILL(vict, SKILL_FIRESHIELD) / 2;
                    hurt(0, 0, vict, nullptr, GET_EQ(ch, WEAR_WIELD1), damdam, 0);
                } else if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) && AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
                           (GET_BONUS(ch, BONUS_FIREPROOF) || IS_DEMON(ch))) {
                    send_to_char(vict, "@RThey appear to be fireproof!@n\r\n");
                }
                pcost(ch, 0, stcost);
            } else {
                if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) && AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
                    !GET_BONUS(ch, BONUS_FIREPROOF) && !IS_DEMON(ch)) {
                    act("@c$N's@W fireshield burns your weapon!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n's@W weapon is burned by your fireshield!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    int damdam = GET_SKILL(vict, SKILL_FIRESHIELD) / 2;
                    hurt(0, 0, vict, nullptr, GET_EQ(ch, WEAR_WIELD2), damdam, 0);
                }
                pcost(ch, 0, stcost);
            }
        }
        if (gun == false && gun2 == false) {
            damage_weapon(ch, weap, vict);
        }
        if (!IS_NPC(ch)) {
            if (PLR_FLAGGED(ch, PLR_THANDW)) {
                if (!GET_SKILL(ch, SKILL_TWOHAND) && slot_count(ch) + 1 <= GET_SLOTS(ch)) {
                    int numb = rand_number(10, 15);
                    SET_SKILL(ch, SKILL_TWOHAND, numb);
                    send_to_char(ch, "@GYou learn the very basics of two-handing your weapon!@n\r\n");
                } else {
                    improve_skill(ch, SKILL_TWOHAND, 0);
                }
            }
        }
        if (GET_EQ(ch, WEAR_WIELD2)) {
            if (!GET_SKILL(ch, SKILL_DUALWIELD) && slot_count(ch) + 1 <= GET_SLOTS(ch) &&
                (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) != ITEM_LIGHT)) {
                int numb = rand_number(10, 15);
                SET_SKILL(ch, SKILL_DUALWIELD, numb);
                send_to_char(ch, "@GYou learn the very basics of dual-wielding!@n\r\n");
            } else {
                improve_skill(ch, SKILL_DUALWIELD, 0);
            }
            if (vict != nullptr && GET_HIT(vict) > 1 && axion_dice(0) < (GET_SKILL(ch, SKILL_DUALWIELD)) &&
                GET_EQ(ch, WEAR_WIELD1)) {
                do_attack2(ch, nullptr, 0, 0);
            }
        }
        handle_multihit(ch, vict);
    } else if (obj) {
        if (!can_kill(ch, nullptr, obj, 0)) {
            return;
        }
        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "It is broken already!\r\n");
            return;
        }
        dmg = ((GET_HIT(ch) / 10000) + (GET_STR(ch)));
        act("@WYou attack $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
        act("@C$n@W attacks $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
        hurt(0, 0, ch, nullptr, obj, dmg, 0);
        if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
            pcost(ch, 0, stcost);

    } else {
        send_to_char(ch, "Error! Please report.\r\n");
        return;
    }
}

ACMD(do_shogekiha) {
    atk::Shogekiha a(ch, argument);
    a.execute();
}

ACMD(do_tsuihidan) {
    atk::Tsuihidan a(ch, argument);
    a.execute();
}

ACMD(do_attack2) {
    int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0, wtype = 0, gun2 = false;
    int dualwield = 0;
    int64_t dmg;
    struct char_data *vict = nullptr;
    struct obj_data *obj = nullptr;
    char arg[MAX_INPUT_LENGTH];
    double attperc = 0;

    one_argument(argument, arg);

    if (!GET_EQ(ch, WEAR_WIELD2)) {
        return;
    }

    int64_t stcost = ((GET_MAX_HIT(ch) / 150) + GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2)));
    int64_t kicost = ((GET_MAX_HIT(ch) / 150) + GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2)));

    if (IS_ANDROID(ch)) {
        stcost *= 0.25;
    }

    if (IS_ANDROID(ch) && gun2 == true) {
        kicost *= 0.25;
    }

    if (!can_grav(ch)) {
        return;
    }

    if (!HAS_ARMS(ch)) {
        send_to_char(ch, "With what arms!?\r\n");
        return;
    } else if (GET_LIMBCOND(ch, 0) > 0 && GET_LIMBCOND(ch, 0) < 50 && GET_LIMBCOND(ch, 1) < 0) {
        send_to_char(ch, "Using your broken right arm has damaged it more!@n\r\n");
        GET_LIMBCOND(ch, 0) -= rand_number(3, 5);
        if (GET_LIMBCOND(ch, 0) < 0) {
            act("@RYour right arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@r$n@R's right arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
    } else if (GET_LIMBCOND(ch, 1) > 0 && GET_LIMBCOND(ch, 1) < 50 && GET_LIMBCOND(ch, 0) < 0) {
        send_to_char(ch, "Using your broken left arm has damaged it more!@n\r\n");
        GET_LIMBCOND(ch, 1) -= rand_number(3, 5);
        if (GET_LIMBCOND(ch, 1) < 0) {
            act("@RYour left arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@r$n@R's left arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
    }

    if (!FIGHTING(ch)) {
        return;
    }

    if (!check_points(ch, 0, stcost)) {
        return;
    }

    if (!IS_NPC(ch) || IS_NPC(ch)) {
        if (vict) {
            if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
                if (!can_kill(ch, vict, nullptr, 1)) {
                    return;
                }
            } else {
                if (!can_kill(ch, vict, nullptr, 0)) {
                    return;
                }
            }
        }

        if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
            skill = init_skill(ch, SKILL_DAGGER);
            improve_skill(ch, SKILL_DAGGER, 1);
            wtype = 1;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
            skill = init_skill(ch, SKILL_SWORD);
            improve_skill(ch, SKILL_SWORD, 1);
            wtype = 0;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
            skill = init_skill(ch, SKILL_CLUB);
            improve_skill(ch, SKILL_CLUB, 1);
            wtype = 2;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
            skill = init_skill(ch, SKILL_SPEAR);
            improve_skill(ch, SKILL_SPEAR, 1);
            wtype = 3;
        } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
            gun2 = true;
            skill = init_skill(ch, SKILL_GUN);
            improve_skill(ch, SKILL_GUN, 1);
            wtype = 4;
        } else {
            skill = init_skill(ch, SKILL_BRAWL);
            improve_skill(ch, SKILL_BRAWL, 1);
            wtype = 5;
        }
    }

    if (gun2 == false) {
        if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 100) {
            dualwield = 3;
            stcost -= stcost * 0.30;
        } else if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 75) {
            dualwield = 2;
            stcost -= stcost * 0.25;
        } else if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 50) {
            dualwield = 1;
            stcost -= stcost * 0.25;
        }
    }

    if (IS_NPC(ch) && GET_LEVEL(ch) <= 10) {
        skill = rand_number(30, 50);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 20) {
        skill = rand_number(30, 60);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 30) {
        skill = rand_number(30, 70);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 50) {
        skill = rand_number(40, 80);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 70) {
        skill = rand_number(50, 90);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 80) {
        skill = rand_number(60, 100);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 90) {
        skill = rand_number(70, 100);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 100) {
        skill = rand_number(80, 100);
    } else if (IS_NPC(ch) && GET_LEVEL(ch) > 100) {
        skill = rand_number(95, 100);
    }
    if (FIGHTING(ch) && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) {
        vict = FIGHTING(ch);
    }

    if (gun2 == true) {
        if (GET_GOLD(ch) < 1) {
            send_to_char(ch, "You do not have enough zenni. You need 1 zenni per shot.\r\n");
            return;
        } else {
            ch->mod(CharMoney::Carried, -1);
        }
    }

    if (GET_PREFERENCE(ch) != PREFERENCE_H2H) {
        handle_cooldown(ch, 4);
    } else {
        handle_cooldown(ch, 8);
    }

    if (vict) {
        if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
            if (!can_kill(ch, vict, nullptr, 1)) {
                return;
            }
        } else {
            if (!can_kill(ch, vict, nullptr, 0)) {
                return;
            }
        }
        index = check_def(vict);
        prob = roll_accuracy(ch, skill, false);
        perc = chance_to_hit(ch);

        index -= handle_speed(ch, vict);

        avo = index / 4;
        handle_defense(vict, &pry, &blk, &dge);
        if (dualwield == 3) {
            pry -= pry * 0.1;
            blk -= pry * 0.1;
            dge -= pry * 0.1; /* pry, This is intentional */
        }

        if (gun2 == true) {
            if (dualwield >= 1) {
                prob += prob * 0.1;
            }
        }

        prob -= avo;
        tech_handle_posmodifier(vict, pry, blk, dge, prob);

        if (!tech_handle_zanzoken(ch, vict, "attack")) {
            if (gun2 == false)
                pcost(ch, 0, stcost / 3);
            pcost(vict, 0, GET_MAX_HIT(vict) / 200);

            return;
        }

        if (prob < perc - 20) {
            if ((vict->getCurST()) > 0) {
                if (pry > rand_number(1, 140) && (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY))) {
                    act("@C$N@W intercepts and parries your attack with $S own!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@WYou intercept and parry @C$n's@W attack with one of your own!@n", true, ch, nullptr, vict,
                        TO_VICT);
                    act("@C$N@W intercepts and parries @c$n's@W attack with one of $S own!@n", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    if (wtype != 4)
                        handle_disarm(ch, vict);
                    improve_skill(vict, SKILL_PARRY, 0);
                    if (gun2 == false)
                        pcost(ch, 0, stcost);
                    pcost(vict, 0, GET_MAX_HIT(vict) / 500);
                    dmg = damtype(vict, -2, skill, attperc);
                    dmg *= calc_critical(ch, 1);
                    hurt(0, 0, vict, ch, nullptr, dmg, -1);

                    return;
                } else if (blk > axion_dice(10)) {
                    act("@C$N@W moves quickly and blocks your attack!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@WYou move quickly and block @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@C$N@W moves quickly and blocks @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    improve_skill(vict, SKILL_BLOCK, 0);
                    if (gun2 == false)
                        pcost(ch, 0, stcost);
                    pcost(vict, 0, GET_MAX_HIT(vict) / 500);
                    dmg = damtype(ch, -1, skill, attperc);
                    dmg /= 4;
                    hurt(0, 0, ch, vict, nullptr, dmg, 0);

                    return;
                } else if (dge > axion_dice(10)) {
                    act("@C$N@W manages to dodge your attack!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@WYou dodge @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@C$N@W manages to dodge @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    improve_skill(vict, SKILL_DODGE, 0);
                    if (gun2 == false)
                        pcost(ch, 0, stcost);
                    hurt(0, 0, ch, vict, nullptr, 0, 0);

                    return;
                } else {
                    act("@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
                    act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    if (gun2 == false)
                        pcost(ch, 0, stcost / 3);
                    hurt(0, 0, ch, vict, nullptr, 0, 0);

                    return;
                }
            } else {
                act("@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
                if (gun2 == false)
                    pcost(ch, 0, stcost / 3);

            }
            hurt(0, 0, ch, vict, nullptr, 0, 0);
            return;
        } else {
            dmg = damtype(ch, -1, skill, attperc);
            int wlvl = 0;
            struct obj_data *weap = GET_EQ(ch, WEAR_WIELD2);
            if (OBJ_FLAGGED(weap, ITEM_WEAPLVL1)) {
                dmg += dmg * .05;
                wlvl = 1;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL2)) {
                dmg += dmg * .1;
                wlvl = 2;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL3)) {
                dmg += dmg * .2;
                wlvl = 3;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL4)) {
                dmg += dmg * .3;
                wlvl = 4;
            } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL5)) {
                dmg += dmg * .5;
                wlvl = 5;
            }
            if (wtype == 5) {
                if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
                    dmg += dmg * 0.5;
                    wlvl = 5;
                } else if (GET_SKILL(ch, SKILL_BRAWL) >= 50) {
                    dmg += dmg * 0.2;
                    wlvl = 3;
                }
            }
            if (wtype == 0 && IS_KONATSU(ch)) {
                dmg += dmg * .25;
            }
            int hitspot = 1;
            if (gun2 == true)
                dmg = gun_dam(ch, wlvl);
            hitspot = roll_hitloc(ch, vict, skill);
            int64_t beforepl = GET_HIT(vict);
            if (wtype == 3) {
                if (skill >= 100)
                    dmg += dmg * 0.04;
                else if (skill >= 50)
                    dmg += dmg * 0.1;
            }
            if (gun2 == true) {
                if (dualwield == 3 && rand_number(1, 3) == 3) {
                    send_to_char(ch, "@GYour masterful aim scores a critical!@n\r\n");
                    hitspot = 2;
                }
            }
            switch (hitspot) {
                case 1:
                    if (GET_BONUS(ch, BONUS_SOFT)) {
                        dmg *= calc_critical(ch, 2);
                    }
                    break;
                case 2: /* Head */
                    hitspot = 4;
                    break;
                case 3: /* Body */
                    hitspot = 5;
                    break;
                case 4: /* Arm */
                    hitspot = 5;
                    break;
                case 5: /* Leg */
                    hitspot = 5;
                    break;
            }
            switch (wtype) {
                case 0:
                    switch (hitspot) {
                        case 1:
                            act("@WYou slash @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou slash @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou slash @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou slash @C$N@W across the face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou slash @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W slashes you across the chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W slashes @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end slash switch*/
                    if (beforepl - GET_HIT(vict) >= (vict->getEffMaxPL()) * 0.025) {
                        cut_limb(ch, vict, wlvl, hitspot);
                    }
                    break;
                case 1:
                    dmg += (dmg * 0.01) * (GET_DEX(ch) * 0.5);
                    switch (hitspot) {
                        case 1:
                            act("@WYou pierce @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou pierce @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N'@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou pierce @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou pierce @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou pierce @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W pierces your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W pierces @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end pierce switch*/
                    break;
                case 2:
                    switch (hitspot) {
                        case 1:
                            act("@WYou crush @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou crush @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou crush @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou crush @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou crush @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W crushes your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W crushes @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end crush switch*/
                    club_stamina(ch, vict, wlvl, dmg);
                    break;
                case 3:
                    switch (hitspot) {
                        case 1:
                            act("@WYou stab @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou stab @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou stab @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou stab @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou stab @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W stabs your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W stabs @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end stab switch*/
                    break;
                case 4:
                    switch (hitspot) {
                        case 1:
                            act("@WYou blast @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou blast @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou blast @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou blast @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 0);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou blast @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W blasts your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W blasts @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end blast switch*/
                    break;
                case 5:
                    switch (hitspot) {
                        case 1:
                            act("@WYou whack @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your stomach!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 2:
                            act("@WYou whack @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your arm!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 1);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 3:
                            act("@WYou whack @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your leg!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            dmg *= calc_critical(ch, 1);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 2);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 4:
                            act("@WYou whack @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your face!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
                                double mult = calc_critical(ch, 0);
                                mult += 1.0;
                                dmg *= mult;
                            } else {
                                dmg *= calc_critical(ch, 0);
                            }
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 3);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                        case 5:
                            act("@WYou whack @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@c$n@W whacks your chest!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$n@W whacks @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 0);
                            dam_eq_loc(vict, 4);
                            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
                            break;
                    }/* end whacks switch*/
                    break;
            }/* end switch one*/
        }
        if (gun2 == false) {
            if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) && AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
                !GET_BONUS(ch, BONUS_FIREPROOF) && !IS_DEMON(ch)) {
                act("@c$N's@W fireshield burns your weapon!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@C$n's@W weapon is burned by your fireshield!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", true, ch, nullptr, vict, TO_NOTVICT);
                int damdam = GET_SKILL(vict, SKILL_FIRESHIELD) / 2;
                hurt(0, 0, vict, nullptr, GET_EQ(ch, WEAR_WIELD2), damdam, 0);
            } else if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) && AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
                       (GET_BONUS(ch, BONUS_FIREPROOF) || IS_DEMON(ch))) {
                send_to_char(vict, "@RThey appear to be fireproof!@n\r\n");
            }
            pcost(ch, 0, stcost);
        }
    } else if (obj) {
        if (!can_kill(ch, nullptr, obj, 0)) {
            return;
        }
        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "It is broken already!\r\n");
            return;
        }
        dmg = ((GET_HIT(ch) / 10000) + (GET_STR(ch)));
        act("@WYou attack $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
        act("@C$n@W attacks $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
        hurt(0, 0, ch, nullptr, obj, dmg, 0);
        if (gun2 == false)
            pcost(ch, 0, stcost);

    } else {
        send_to_char(ch, "Error! Please report.\r\n");
        return;
    }
}

ACMD(do_bite) {
    atk::Bite a(ch, argument);
    a.execute();
}

ACMD(do_kiball) {
    atk::KiBall a(ch, argument);
    a.execute();
}

ACMD(do_beam) {
    atk::Beam a(ch, argument);
    a.execute();
}

ACMD(do_kiblast) {
    atk::KiBlast a(ch, argument);
    a.execute();
}

ACMD(do_slam) {
    atk::Slam a(ch, argument);
    a.execute();
}

ACMD(do_uppercut) {
    atk::Uppercut a(ch, argument);
    a.execute();
}

ACMD(do_roundhouse) {
    atk::Roundhouse a(ch, argument);
    a.execute();
}

ACMD(do_elbow) {
    atk::Elbow a(ch, argument);
    a.execute();
}

ACMD(do_kick) {
    atk::Kick a(ch, argument);
    a.execute();
}

ACMD(do_knee) {
    atk::Knee a(ch, argument);
    a.execute();
}

ACMD(do_punch) {
    atk::Punch a(ch, argument);
    a.execute();
}

ACMD(do_tailwhip) {
    atk::Tailwhip a(ch, argument);
    a.execute();
}

/* This handles charging for energy attacks */
ACMD(do_charge) {

    char arg[MAX_INPUT_LENGTH];
    int amt;

    one_argument(argument, arg);

    if (PLR_FLAGGED(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "@WYou are concentrating too much on your aura to be able to charge.");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        send_to_char(ch, "You are inside a healing tank!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_MBREAK)) {
        send_to_char(ch, "Your mind is still strained from psychic attacks...\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_POISON)) {
        send_to_char(ch, "You feel too sick from the poison to concentrate.\r\n");
        return;
    }
    if (!*arg) {
        send_to_char(ch, "Charge, yes. How much percent though?\r\n");
        send_to_char(ch, "[ 1 - 100 | cancel | release]\r\n");
        return;
    } else if (!strcasecmp("release", arg) && (PLR_FLAGGED(ch, PLR_CHARGE) && GET_CHARGE(ch) <= 0)) {
        send_to_char(ch, "Try cancel instead, you have nothing charged up yet!\r\n");
        return;
    } else if (!strcasecmp("release", arg) && (PLR_FLAGGED(ch, PLR_CHARGE) && GET_CHARGE(ch) > 0)) {
        send_to_char(ch, "You stop charging and release your pent up energy.\r\n");
        switch (rand_number(1, 3)) {
            case 1:
                act("$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            case 2:
                act("$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            case 3:
                act("$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            default:
                act("$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
        }
        ch->incCurKI(GET_CHARGE(ch));
        GET_CHARGE(ch) = 0;
        GET_CHARGETO(ch) = 0;
        ch->playerFlags.reset(PLR_CHARGE);
        return;
    } else if (!strcasecmp("release", arg) && GET_CHARGE(ch) > 0) {
        send_to_char(ch, "You release your pent up energy.\r\n");
        switch (rand_number(1, 3)) {
            case 1:
                act("$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            case 2:
                act("$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            case 3:
                act("$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            default:
                act("$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
        }
        ch->incCurKI(GET_CHARGE(ch));
        GET_CHARGE(ch) = 0;
        GET_CHARGETO(ch) = 0;
        return;
    } else if (!strcasecmp("cancel", arg) && PLR_FLAGGED(ch, PLR_CHARGE)) {
        send_to_char(ch, "You stop charging.\r\n");
        switch (rand_number(1, 3)) {
            case 1:
                act("$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            case 2:
                act("$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            case 3:
                act("$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
            default:
                act("$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
                break;
        }
        ch->playerFlags.reset(PLR_CHARGE);
        GET_CHARGETO(ch) = 0;
        return;
    } else if (!strcasecmp("cancel", arg) && !PLR_FLAGGED(ch, PLR_CHARGE)) {
        send_to_char(ch, "You are not even charging!\r\n");
        return;
    } else if ((ch->getCurKI()) < GET_MAX_MANA(ch) / 100) {
        send_to_char(ch, "You don't even have enough ki!\r\n");
        return;
    } else if ((amt = atoi(arg)) > 0) {
        if (amt >= 101) {
            send_to_char(ch, "You have set it too high!\r\n");
            return;
        } else if (AFF_FLAGGED(ch, AFF_SPIRITCONTROL)) {
            int64_t diff = 0;
            if ((ch->getCurKI()) < ((GET_MAX_MANA(ch) * 0.01) * amt) + 1) {
                diff = (((GET_MAX_MANA(ch) * 0.01) * amt) + 1) - (ch->getCurKI());
            }
            int chance = 15;
            chance -= GET_SKILL(ch, SKILL_SPIRITCONTROL) / 10;
            if (chance < 10)
                chance = 10;
            else if (chance > 15)
                chance = 15;

            if (chance > rand_number(1, 100)) {
                send_to_char(ch,
                             "The rush of ki that you try to pool temporarily overwhelms you and you lose control!\r\n");
                null_affect(ch, AFF_SPIRITCONTROL);
                return;
            } else {
                int64_t spiritcost = GET_MAX_MANA(ch) * 0.05;
                if (GET_SKILL(ch, SKILL_SPIRITCONTROL) >= 100) {
                    spiritcost = GET_MAX_MANA(ch) * 0.01;
                }
                reveal_hiding(ch, 0);
                send_to_char(ch,
                             "Your %s colored aura flashes up around you as you instantly take control of the ki you needed!\r\n",
                             aura_types[GET_AURA(ch)]);
                send_to_char(ch, "@D[@RCost@D: @r%s@D]@n\r\n", add_commas(spiritcost));
                char bloom[MAX_INPUT_LENGTH];
                sprintf(bloom, "@wA %s aura flashes up brightly around $n@w!@n", aura_types[GET_AURA(ch)]);
                act(bloom, true, ch, nullptr, nullptr, TO_ROOM);
                GET_CHARGE(ch) = (((GET_MAX_MANA(ch) * 0.01) * amt) + 1) - diff;
                ch->decCurKI((((GET_MAX_MANA(ch) * 0.01) * amt) + 1) - diff + spiritcost);
            }
        } else {
            reveal_hiding(ch, 0);
            send_to_char(ch, "You begin to charge some energy, as a %s aura begins to burn around you!\r\n",
                         aura_types[GET_AURA(ch)]);
            char bloom[MAX_INPUT_LENGTH];
            sprintf(bloom, "@wA %s aura flashes up brightly around $n@w!@n", aura_types[GET_AURA(ch)]);
            act(bloom, true, ch, nullptr, nullptr, TO_ROOM);
            GET_CHARGETO(ch) = (((GET_MAX_MANA(ch) * 0.01) * amt) + 1);
            GET_CHARGE(ch) += 1;
            ch->playerFlags.set(PLR_CHARGE);
        }
    } else if (amt < 1 && GET_ROOM_VNUM(IN_ROOM(ch)) != 1562) {
        send_to_char(ch, "You have set it too low!\r\n");
        return;
    } else {
        send_to_char(ch, "That is an invalid argument.\r\n");
    }
}

ACMD(do_powerup) {
    if (IS_NPC(ch)) {
        ch->mobFlags.set(MOB_POWERUP);
        if (GET_MAX_HIT(ch) < 50000) {
            act("@RYou begin to powerup, and air billows outward around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@R$n begins to powerup, and air billows outward around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 500000) {
            act("@RYou begin to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 5000000) {
            act("@RYou begin to powerup, and torrents of energy crackle around you!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and torrents of energy crackle around $m!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 50000000) {
            act("@RYou begin to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 100000000) {
            act("@RYou begin to powerup, and massive cracks begin to form beneath you!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and massive cracks begin to form beneath $m!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 300000000) {
            act("@RYou begin to powerup, and everything around you shudders from the power!@n", true, ch, nullptr,
                nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and everything around $m shudders from the power!@n", true, ch, nullptr,
                nullptr, TO_ROOM);
        } else {
            act("@RYou begin to powerup, and the very air around you begins to burn!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and the very air around $m begins to burn!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        }
        return;
    }
    if (PLR_FLAGGED(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "@WYou are concentrating too much on your aura to be able to power up.");
        return;
    }
    if (IS_ANDROID(ch)) {
        send_to_char(ch, "@WYou are an android, you do not powerup.@n");
        return;
    }
    if (GET_SUPPRESS(ch) > 0) {
        send_to_char(ch, "@WYou currently have your powerlevel suppressed to %" I64T " percent.@n", GET_SUPPRESS(ch));
        return;
    }
    if (PLR_FLAGGED(ch, PLR_POWERUP)) {
        send_to_char(ch, "@WYou stop powering up.@n");
        ch->playerFlags.reset(PLR_POWERUP);
        return;
    }
    if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
        send_to_char(ch, "@WYou are already at max!@n");
        return;
    }
    if ((ch->getCurKI()) < GET_MAX_MANA(ch) / 20) {
        send_to_char(ch, "@WYou do not have enough ki to powerup!@n");
        return;
    } else {
        reveal_hiding(ch, 0);
        if (GET_MAX_HIT(ch) < 50000) {
            act("@RYou begin to powerup, and air billows outward around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@R$n begins to powerup, and air billows outward around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 500000) {
            act("@RYou begin to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 5000000) {
            act("@RYou begin to powerup, and torrents of energy crackle around you!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and torrents of energy crackle around $m!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 50000000) {
            act("@RYou begin to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 100000000) {
            act("@RYou begin to powerup, and massive cracks begin to form beneath you!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and massive cracks begin to form beneath $m!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        } else if (GET_MAX_HIT(ch) < 300000000) {
            act("@RYou begin to powerup, and everything around you shudders from the power!@n", true, ch, nullptr,
                nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and everything around $m shudders from the power!@n", true, ch, nullptr,
                nullptr, TO_ROOM);
        } else {
            act("@RYou begin to powerup, and the very air around you begins to burn!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@R$n begins to powerup, and the very air around $m begins to burn!@n", true, ch, nullptr, nullptr,
                TO_ROOM);
        }
        ch->playerFlags.set(PLR_POWERUP);
        return;
    }
}

ACMD(do_rescue) {

    char arg[100];
    struct char_data *helpee, *opponent;

    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Whom do you wish to rescue?\r\n");
    else if (!(helpee = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
        send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (helpee == ch)
        send_to_char(ch, "You can't help yourself any more than this!\r\n");
    else if (!FIGHTING(helpee))
        send_to_char(ch, "They are not fighting anyone!\r\n");
    else if (FIGHTING(ch) && !IS_NPC(ch))
        send_to_char(ch, "You are a little too busy fighting for yourself!\r\n");
    else {
        opponent = FIGHTING(helpee);
        int mobbonus = 0;

        if (IS_NPC(ch)) {
            mobbonus = GET_SPEEDI(ch) * 0.2;
        }
        if (GET_SPEEDI(ch) + mobbonus < GET_SPEEDI(opponent) && rand_number(1, 3) != 3) {
            act("@GYou leap towards @g$N@G and try to rescue $M but are too slow!@n", true, ch, nullptr, helpee,
                TO_CHAR);
            act("@g$n@G leaps towards you! $n is too slow and fails to rescue you!@n", true, ch, nullptr, helpee,
                TO_VICT);
            act("@g$n@G leaps towards @g$N@G and tries to rescue $M but is too slow!@n", true, ch, nullptr, helpee,
                TO_NOTVICT);
            return;
        }
        act("@GYou leap in front of @g$N@G and rescue $M!@n", true, ch, nullptr, helpee, TO_CHAR);
        act("@g$n@G leaps in front of you! You are rescued!@n", true, ch, nullptr, helpee, TO_VICT);
        act("@g$n@G leaps in front of @g$N@G and rescues $M!@n", true, ch, nullptr, helpee, TO_NOTVICT);
        stop_fighting(opponent);
        hurt(0, 0, ch, opponent, nullptr, rand_number(1, GET_LEVEL(ch)), 1);
        hurt(0, 0, opponent, ch, nullptr, rand_number(1, GET_LEVEL(ch)), 1);
        return;
    }
}

ACMD(do_assist) {
    char arg[MAX_INPUT_LENGTH];
    struct char_data *helpee, *opponent;

    if (FIGHTING(ch)) {
        send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
        return;
    }
    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Whom do you wish to assist?\r\n");
    else if (!(helpee = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
        send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (helpee == ch)
        send_to_char(ch, "You can't help yourself any more than this!\r\n");
    else {
        /*
     * Hit the same enemy the person you're helping is.
     */
        if (FIGHTING(helpee))
            opponent = FIGHTING(helpee);
        else
            for (opponent = ch->getRoom()->people;
                 opponent && (FIGHTING(opponent) != helpee);
                 opponent = opponent->next_in_room);

        if (!opponent)
            act("But nobody is fighting $M!", true, ch, nullptr, helpee, TO_CHAR);
        else if (!CAN_SEE(ch, opponent))
            act("You can't see who is fighting $M!", true, ch, nullptr, helpee, TO_CHAR);
            /* prevent accidental pkill */
        else {
            reveal_hiding(ch, 0);
            send_to_char(ch, "You join the fight!\r\n");
            act("$N assists you!", 0, helpee, nullptr, ch, TO_CHAR);
            act("$n assists $N.", true, ch, nullptr, helpee, TO_NOTVICT);
            if (!FIGHTING(ch)) {
                set_fighting(ch, opponent);
            }
            if (!FIGHTING(opponent)) {
                set_fighting(opponent, ch);
            }
        }
    }
}

ACMD(do_kill) {
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;

    if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_INSTANTKILL)) {
        return;
    }
    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Kill who?\r\n");
    } else {
        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
            send_to_char(ch, "They aren't here.\r\n");
        else if (ch == vict)
            send_to_char(ch, "Your mother would be so sad.. :(\r\n");
        else {
            act("You chop $M to pieces!  Ah!  The blood!", true, ch, nullptr, vict, TO_CHAR);
            act("$N chops you to pieces!", true, vict, nullptr, ch, TO_CHAR);
            act("$n brutally slays $N!", true, ch, nullptr, vict, TO_NOTVICT);
            raw_kill(vict, ch);
        }
    }
}

ACMD(do_flee) {
    int i, attempt;
    struct char_data *was_fighting;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (GET_POS(ch) < POS_RESTING) {
        send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
        return;
    }

    if (GRAPPLING(ch)) {
        send_to_char(ch, "You are grappling with someone!\r\n");
        return;
    }

    if (GRAPPLED(ch)) {
        send_to_char(ch, "You are grappling with someone!\r\n");
        return;
    }

    if (ABSORBING(ch)) {
        send_to_char(ch, "You are absorbing from someone!\r\n");
        return;
    }

    if (ABSORBBY(ch)) {
        send_to_char(ch, "You are being absorbed from by someone!\r\n");
        return;
    }

    if (!IS_NPC(ch)) {
        int fail = false;
        auto isMyAttack = [&](const auto&o) {return KICHARGE(o) > 0 && USER(o) == ch;};
        if (auto obj = ch->getRoom()->findObject(isMyAttack); obj) {
            send_to_char(ch, "You are too busy controlling your attack!\r\n");
            return;
        }
    }

    for (i = 0; i < 12; i++) {
        if (*arg) {
            if ((attempt = search_block(arg, dirs, false) > -1)) {
                attempt = search_block(arg, dirs, false);
            } else if ((attempt = search_block(arg, abbr_dirs, false) > -1)) {
                attempt = search_block(arg, abbr_dirs, false);
            } else {
                attempt = rand_number(0, NUM_OF_DIRS - 1);  /* Select a random direction */
            }
        }
        if (!*arg) {
            attempt = rand_number(0, NUM_OF_DIRS - 1);    /* Select a random direction */
        }
        if (CAN_GO(ch, attempt) &&
            !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
            act("$n panics, and attempts to flee!", true, ch, nullptr, nullptr, TO_ROOM);
            if (IS_NPC(ch) && ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_NOMOB)) {
                return;
            }
            was_fighting = FIGHTING(ch);

            auto isWall = [&](const auto&o) {return o->vn == 79 && GET_OBJ_COST(o) == attempt;};
            if(auto wall = ch->getRoom()->findObject(isWall); wall) {
                send_to_char(ch, "That direction has a glacial wall blocking it.\r\n");
                return;
            }

            if (!block_calc(ch)) {
                return;
            }

            if (ABSORBING(ch)) {
                send_to_char(ch, "You are busy absorbing from %s!\r\n", GET_NAME(ABSORBING(ch)));
                return;
            }
            if (ABSORBBY(ch)) {
                if (axion_dice(0) < GET_SKILL(ABSORBING(ch), SKILL_ABSORB)) {
                    send_to_char(ch, "You are being held by %s, they are absorbing you!\r\n", GET_NAME(ABSORBBY(ch)));
                    send_to_char(ABSORBBY(ch), "%s struggles in your grasp!\r\n", GET_NAME(ch));
                    WAIT_STATE(ch, PULSE_2SEC);
                    return;
                } else {
                    act("@c$N@W manages to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch,
                        TO_NOTVICT);
                    act("@WYou manage to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_VICT);
                    act("@c$N@W manages to break loose of your hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_CHAR);
                    ABSORBING(ABSORBBY(ch)) = nullptr;
                    ABSORBBY(ch) = nullptr;
                }
            }
            if (do_simple_move(ch, attempt, true)) {
                send_to_char(ch, "You flee head over heels.\r\n");
                WAIT_STATE(ch, PULSE_2SEC);
            } else {
                act("$n tries to flee, but can't!", true, ch, nullptr, nullptr, TO_ROOM);
                WAIT_STATE(ch, PULSE_2SEC);
            }
            return;
        }
    }
    send_to_char(ch, "PANIC!  You couldn't escape!\r\n");

}
