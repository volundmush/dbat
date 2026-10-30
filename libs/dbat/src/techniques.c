//
// Created by volund on 11/4/21.
//

#include "dbat/techniques.h"
#include "dbat/utils.h"
#include "dbat/combat.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/character_utils.h"

bool tech_handle_zanzoken(char_data *ch, char_data *vict, const char* name) {
    if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
            (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
        if (!char_aff_flagged(ch, AFF_ZANZOKEN) || (char_aff_flagged(ch, AFF_ZANZOKEN) && char_get_speedi(ch) + rand_number(1, 5) < char_get_speedi(vict) + rand_number(1, 5))) {
            char msg[MAX_INPUT_LENGTH];
            snprintf(msg, sizeof(msg), "@C$N@c disappears, avoiding your %s before reappearing!@n", name);
            act(msg, TRUE, ch, nullptr, vict, TO_CHAR);
            snprintf(msg, sizeof(msg), "@cYou disappear, avoiding @C$n's@c %s before reappearing!@n", name);
            act(msg, TRUE, ch, nullptr, vict, TO_VICT);
            snprintf(msg, sizeof(msg), "@C$N@c disappears, avoiding @C$n's@c %s before reappearing!@n", name);
            act(msg, TRUE, ch, nullptr, vict, TO_NOTVICT);
            if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
                REMOVE_BIT_AR(char_get_affected_by(ch), AFF_ZANZOKEN);
            }
            REMOVE_BIT_AR(char_get_affected_by(vict), AFF_ZANZOKEN);
            return false;
        }
        else {
            act("@C$N@c disappears, trying to avoid your attack but your zanzoken is faster!@n", FALSE, ch, 0, vict, TO_CHAR);
            act("@cYou zanzoken to avoid the attack but @C$n's@c zanzoken is faster!@n", FALSE, ch, 0, vict, TO_VICT);
            act("@C$N@c disappears, trying to avoid @C$n's@c attack but @C$n's@c zanzoken is faster!@n", FALSE, ch, 0, vict, TO_NOTVICT);
            REMOVE_BIT_AR(char_get_affected_by(vict), AFF_ZANZOKEN);
            REMOVE_BIT_AR(char_get_affected_by(ch), AFF_ZANZOKEN);
        }
    }
    return true;
}

void tech_handle_posmodifier(char_data *vict, int *pry, int *blk, int *dge, int *prob) {
    switch(char_get_position(vict)) {
        case POS_SLEEPING:
            *pry = 0;
            *blk = 0;
            *dge = 0;
            *prob += 50;
        case POS_RESTING:
            *pry /= 4;
            *blk /= 4;
            *dge /= 4;
            *prob += 25;
        case POS_SITTING:
            *pry /= 2;
            *blk /= 2;
            *dge /= 2;
            *prob += 10;
    }
}

bool tech_handle_charge(char_data *ch, char *arg, double minimum, double *attperc) {
    if (*arg) {
        double adjust = (double)(atoi(arg)) * 0.01;
        if (adjust < 0.01 || adjust > 1.00) {
            send_to_char(ch, "If you are going to supply a percentage of your charge to use then use an acceptable number (1-100)\r\n");
            return false;
        } else if (adjust < *attperc && adjust >= minimum) {
            *attperc = adjust;
        } else if (adjust < minimum) {
            *attperc = minimum;
        }
    }
    return true;
}

bool tech_handle_targeting(char_data *ch, char *arg, char_data **vict, obj_data **obj) {
    *vict = nullptr; *obj = nullptr;
    if (!*arg || !(*vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM))) {
        if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
            *vict = char_get_fighting(ch);
            return true;
        } else if (!(*obj = get_obj_in_list_vis(ch, arg, nullptr, world[char_get_in_room(ch)].contents))) {
            send_to_char(ch, "Nothing around here by that name.\r\n");
            return false;
        }
        return true;
    }
    return true;
}


void tech_handle_fireshield(char_data *ch, char_data *vict, const char* part) {

    const char *msg = part ? part : "body";
    if (GET_HIT(vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
        char msg[MAX_INPUT_LENGTH];
        snprintf(msg, sizeof(msg), "@c$N's@W fireshield burns your %s!@n", msg);
        act(msg, TRUE, ch, nullptr, vict, TO_CHAR);
        snprintf(msg, sizeof(msg), "@C$n's@W %s is burned by your fireshield!@n", msg);
        act(msg, TRUE, ch, nullptr, vict, TO_VICT);
        snprintf(msg, sizeof(msg), "@c$n's@W %s is burned by @C$N's@W fireshield!@n", msg);
        act(msg, TRUE, ch, nullptr, vict, TO_NOTVICT);
        int64_t dmg = GET_MAX_MANA(vict) * 0.02;
        char_set_lastattack(vict, char_get_lastattack(vict) + (1000));
        hurt(0, 0, vict, ch, nullptr, dmg, 0);
        if (char_get_bonuses(ch, BONUS_FIREPRONE)) {
            send_to_char(ch, "@RYou are extremely flammable and are burned by the attack!@n\r\n");
            send_to_char(vict, "@RThey are easily burned!@n\r\n");
            SET_BIT_AR(char_get_affected_by(ch), AFF_BURNED);
        } else if (char_get_aff_abils_con(ch) < axion_dice(0)) {
            send_to_char(ch, "@RYou are badly burned!@n\r\n");
            send_to_char(vict, "@RThey are burned!@n\r\n");
            SET_BIT_AR(char_get_affected_by(ch), AFF_BURNED);
        }
    } else if (GET_HIT(vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && (char_get_bonuses(ch, BONUS_FIREPROOF) || char_is_demon(ch))) {
        send_to_char(vict, "@RThey appear to be fireproof!@n\r\n");
    }
}

bool tech_handle_android_absorb(char_data *ch, char_data *vict) {
    if (char_is_android(vict) && char_has_arms(vict) && char_get_skill(vict, SKILL_ABSORB) > rand_number(1, 140)) {
        act("@C$N@W absorbs your ki attack and all your charged ki with $S hand!@n", TRUE, ch, nullptr, vict, TO_CHAR);
        act("@WYou absorb @C$n's@W ki attack and all $s charged ki with your hand!@n", TRUE, ch, nullptr, vict, TO_VICT);
        act("@C$N@W absorbs @c$n's@W ki attack and all $s charged ki with $S hand!@n", TRUE, ch, nullptr, vict, TO_NOTVICT);
        int amot = char_get_charge(ch);
        if (char_is_npc(ch)) {
            amot = GET_MAX_MANA(ch) / 20;
        }
        if (char_get_charge(vict) + amot > GET_MAX_MANA(vict)) {
            incCurKI(vict, getMaxKI(vict) - char_get_charge(vict), true);
            char_set_charge(vict, GET_MAX_MANA(vict));
        }
        else {
            char_set_charge(vict, char_get_charge(vict) + (amot));
        }
        return true;
    }
    return false;
}

void tech_handle_crashdown(char_data *ch, char_data *vict) {
    if (char_aff_flagged(vict, AFF_FLYING)) {
        act("@w$N@w is knocked out of the air!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@wYou are knocked out of the air!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@w$N@w is knocked out of the air!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        REMOVE_BIT_AR(char_get_affected_by(vict), AFF_FLYING);
        char_set_altitude(vict, 0);
        char_set_position(vict, POS_SITTING);
    }
    else {
        handle_knockdown(vict);
    }
}