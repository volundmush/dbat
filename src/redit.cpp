/************************************************************************
 *  OasisOLC - Rooms / redit.c					v2.0	*
 *  Original author: Levork						*
 *  Copyright 1996 Harvey Gilpin					*
 *  Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/db.h"
#include "dbat/boards.h"
#include "dbat/genolc.h"
#include "dbat/genwld.h"
#include "dbat/genzon.h"
#include "dbat/oasis.h"
#include "dbat/improved-edit.h"
#include "dbat/dg_olc.h"
#include "dbat/constants.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/races.h"
#include "dbat/act.wizard.h"
#include "dbat/act.informative.h"

/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */

/*------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

ACMD(do_oasis_redit) {

}

void redit_setup_new(struct descriptor_data *d) {

}

/*------------------------------------------------------------------------*/

void redit_setup_existing(struct descriptor_data *d, int real_num) {

}

/*------------------------------------------------------------------------*/

void redit_save_internally(struct descriptor_data *d) {

}

/*------------------------------------------------------------------------*/

void redit_save_to_disk(zone_vnum zone_num) {

}

/*------------------------------------------------------------------------*/

void free_room(Room *room) {

}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(struct descriptor_data *d) {

}

/*
 * For exits.
 */
void redit_disp_exit_menu(struct descriptor_data *d) {

}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(struct descriptor_data *d) {

}

/*
 * For room flags.
 */
void redit_disp_flag_menu(struct descriptor_data *d) {

}

/*
 * For sector type.
 */
void redit_disp_sector_menu(struct descriptor_data *d) {

}

/*
 * The main menu.
 */
void redit_disp_menu(struct descriptor_data *d) {

}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse(struct descriptor_data *d, char *arg) {

}

void redit_string_cleanup(struct descriptor_data *d, int terminator) {

}
