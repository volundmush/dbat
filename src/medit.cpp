/************************************************************************
 * OasisOLC - Mobiles / medit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/medit.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/genolc.h"
#include "dbat/genmob.h"
#include "dbat/genzon.h"
#include "dbat/genshp.h"
#include "dbat/oasis.h"
#include "dbat/constants.h"
#include "dbat/improved-edit.h"
#include "dbat/dg_olc.h"
#include "dbat/races.h"
#include "dbat/class.h"
#include "dbat/act.wizard.h"
#include "dbat/modify.h"
/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

ACMD(do_oasis_medit) {

}

void medit_save_to_disk(zone_vnum foo) {

}

void medit_setup_new(struct descriptor_data *d) {

}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num) {

}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(struct char_data *mob) {

    //GET_HIT(mob) = 0;
    //GET_MAX_MANA(mob) = 0;
    GET_NDD(mob) = 0;
    mob->set(CharAppearance::Sex, SEX_MALE);
    mob->chclass = SenseiID::Commoner;

    GET_WEIGHT(mob) = rand_number(100, 200);
    mob->setHeight(rand_number(100, 200));

    auto base1 = rand_number(8, 16);
    auto base2 = rand_number(8, 16);
    for(auto attr : {CharAttribute::Strength, CharAttribute::Intelligence, CharAttribute::Wisdom}) {
        mob->set(attr, base1);
    }

    for(auto attr : {CharAttribute::Agility, CharAttribute::Constitution, CharAttribute::Speed}) {
        mob->set(attr, base2);
    }

    mob->mobFlags.set(MOB_ISNPC);
}

/*-------------------------------------------------------------------*/

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(struct descriptor_data *d) {

}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(struct descriptor_data *d) {
    int i;

    clear_screen(d);

    for (i = 0; *position_types[i] != '\n'; i++) {
        write_to_output(d, "@g%2d@n) %s\r\n", i, position_types[i]);
    }
    write_to_output(d, "Enter position number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(struct descriptor_data *d) {
    int i;

    clear_screen(d);

    for (i = 0; i < NUM_GENDERS; i++) {
        write_to_output(d, "@g%2d@n) %s\r\n", i, genders[i]);
    }
    write_to_output(d, "Enter gender number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(struct descriptor_data *d) {
    int i, columns = 0;
    char flags[MAX_STRING_LENGTH];

    clear_screen(d);
    for (i = 0; i < NUM_MOB_FLAGS; i++) {
        write_to_output(d, "@g%2d@n) %-20.20s  %s", i + 1, action_bits[i],
                        !(++columns % 2) ? "\r\n" : "");
    }
    sprintbitarray(OLC_MOB(d)->mobFlags, action_bits, AF_ARRAY_MAX, flags);
    write_to_output(d, "\r\nCurrent flags : @c%s@n\r\nEnter mob flags (0 to quit) : ",
                    flags);
}

/*-------------------------------------------------------------------*/

void medit_disp_personality(struct descriptor_data *d) {

    write_to_output(d, "@GPersonalities\n");
    write_to_output(d, "@D--------------@n\n");
    write_to_output(d, "@w1@D) @WBasic@n\n");
    write_to_output(d, "@w1@D) @WCareful@n\n");
    write_to_output(d, "@w1@D) @WAggressive@n\n");
    write_to_output(d, "@w1@D) @WArrogant\n");
    write_to_output(d, "@w1@D) @WIntelligent@n\n");

}

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(struct descriptor_data *d) {
    int i, columns = 0;
    char flags[MAX_STRING_LENGTH];

    clear_screen(d);
    for (i = 0; i < NUM_AFF_FLAGS; i++) {
        write_to_output(d, "@g%2d@n) %-20.20s  %s", i + 1, affected_bits[i + 1],
                        !(++columns % 2) ? "\r\n" : "");
    }
    sprintbitarray(AFF_FLAGS(OLC_MOB(d)), affected_bits, AF_ARRAY_MAX, flags);
    write_to_output(d, "\r\nCurrent flags   : @c%s@n\r\nEnter aff flags (0 to quit) : ",
                    flags);
}

/*-------------------------------------------------------------------*/

/*
 * Display class menu.
 */
void medit_disp_class(struct descriptor_data *d) {
    int i;
    char buf[MAX_INPUT_LENGTH];
    clear_screen(d);
    auto check = [](SenseiID id) {return true;};

    for (const auto cl: sensei::filterSenseis(check)) {
        sprintf(buf, "@g%2d@n) %s\r\n", cl, sensei::getName(cl).c_str());
        write_to_output(d, buf);
    }
    write_to_output(d, "Enter class number : ");
}
/*-------------------------------------------------------------------*/
/*
 * Display race menu.
 */
void medit_disp_race(struct descriptor_data *d) {
    int columns = 0;
    char buf[MAX_INPUT_LENGTH];

    clear_screen(d);
    auto check = [](RaceID id) {return true;};
    for (const auto &r: race::filterRaces(check)) {
        sprintf(buf, "@g%2d@n) %-20.20s  %s", r, race::getName(r).c_str(),
                !(++columns % 2) ? "\r\n" : "");
        write_to_output(d, buf);
    }
    write_to_output(d, "Enter race number : ");
}

/*-------------------------------------------------------------------*/
/*
 * Display size menu.
 */
void medit_disp_size(struct descriptor_data *d) {

}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d) {

}

/************************************************************************
 *			The GARGANTAUN event handler			*
 ************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg) {

}

void medit_string_cleanup(struct descriptor_data *d, int terminator) {

}

