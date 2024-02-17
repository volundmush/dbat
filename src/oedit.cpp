/************************************************************************
 * OasisOLC - Objects / oedit.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/structs.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/boards.h"
#include "dbat/constants.h"
#include "dbat/shop.h"
#include "dbat/genolc.h"
#include "dbat/genobj.h"
#include "dbat/genzon.h"
#include "dbat/oasis.h"
#include "dbat/improved-edit.h"
#include "dbat/dg_olc.h"
#include "dbat/feats.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/races.h"
#include "dbat/fight.h"

/*------------------------------------------------------------------------*/

/*
 * Handy macros.
 */
#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

ACMD(do_oasis_oedit) {

}

void oedit_setup_new(struct descriptor_data *d) {

}

/*------------------------------------------------------------------------*/

void oedit_setup_existing(struct descriptor_data *d, int real_num) {

}

/*------------------------------------------------------------------------*/

void oedit_save_internally(struct descriptor_data *d) {

}

/*------------------------------------------------------------------------*/

void oedit_save_to_disk(int zone_num) {

}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For container flags.
 */
void oedit_disp_container_flags_menu(struct descriptor_data *d) {

}

/*
 * Ask for *which* apply to edit.
 */
void oedit_disp_prompt_apply_menu(struct descriptor_data *d) {

}

void oedit_disp_prompt_spellbook_menu(struct descriptor_data *d) {

}

void oedit_disp_spellbook_menu(struct descriptor_data *d) {

}


/*
 * Some applies require parameters (skills, feats)
 */
void oedit_disp_apply_spec_menu(struct descriptor_data *d) {

}

/*
 * Ask for liquid type.
 */
void oedit_liquid_type(struct descriptor_data *d) {

}

/*
 * The actual apply to set.
 */
void oedit_disp_apply_menu(struct descriptor_data *d) {

}

/*
 * Weapon critical type.
 */
void oedit_disp_crittype_menu(struct descriptor_data *d) {

}

/*
 * Weapon type.
 */
void oedit_disp_weapon_menu(struct descriptor_data *d) {

}

/*
 * Armor type.
 */
void oedit_disp_armor_menu(struct descriptor_data *d) {

}

/*
 * Spell type.
 */
void oedit_disp_spells_menu(struct descriptor_data *d) {

}

/*
 * Material type.
 */
void oedit_disp_material_menu(struct descriptor_data *d) {

}

/*
 * Object value #1
 */
void oedit_disp_val1_menu(struct descriptor_data *d) {

}

/*
 * Object value #2
 */
void oedit_disp_val2_menu(struct descriptor_data *d) {

}

/*
 * Object value #3
 */
void oedit_disp_val3_menu(struct descriptor_data *d) {

}

/*
 * Object value #4
 */
void oedit_disp_val4_menu(struct descriptor_data *d) {

}

/*
 * Object value #5
 */
void oedit_disp_val5_menu(struct descriptor_data *d) {

}

/*
 * Object value #7
 */
void oedit_disp_val7_menu(struct descriptor_data *d) {

}

/*
 * Object value #9
 */
void oedit_disp_val9_menu(struct descriptor_data *d) {

}

/*
 * Object type.
 */
void oedit_disp_type_menu(struct descriptor_data *d) {

}

/*
 * Object extra flags.
 */
void oedit_disp_extra_menu(struct descriptor_data *d) {

}

/*
 * Object perm flags.
 */
void oedit_disp_perm_menu(struct descriptor_data *d) {

}

/*
 * Object size
 */
void oedit_disp_size_menu(struct descriptor_data *d) {

}

/*
 * Object wear flags.
 */
void oedit_disp_wear_menu(struct descriptor_data *d) {

}

/*
 * Display main menu.
 */
void oedit_disp_menu(struct descriptor_data *d) {

}

/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/

void oedit_parse(struct descriptor_data *d, char *arg) {

}

void oedit_string_cleanup(struct descriptor_data *d, int terminator) {

}

/* this is all iedit stuff */
void iedit_setup_existing(struct descriptor_data *d, Object *real_num) {

}

ACMD(do_iedit) {

}


