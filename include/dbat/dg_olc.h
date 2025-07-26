/**************************************************************************
*  File: dg_olc.h                                                         *
*                                                                         *
*  Usage: this source file is used in extending Oasis style OLC for       *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#pragma once

#include "dg_scripts.h"


constexpr int NUM_TRIG_TYPE_FLAGS = 22;

/*
 * Submodes of TRIGEDIT connectedness.
 */
constexpr int TRIGEDIT_MAIN_MENU = 0;
constexpr int TRIGEDIT_TRIGTYPE = 1;
constexpr int TRIGEDIT_CONFIRM_SAVESTRING = 2;
constexpr int TRIGEDIT_NAME = 3;
constexpr int TRIGEDIT_INTENDED = 4;
constexpr int TRIGEDIT_TYPES = 5;
constexpr int TRIGEDIT_COMMANDS = 6;
constexpr int TRIGEDIT_NARG = 7;
constexpr int TRIGEDIT_ARGUMENT = 8;
constexpr int TRIGEDIT_COPY = 9;

constexpr int OLC_SCRIPT_EDIT = 82766;  /* arbitrary > highest possible room number */
constexpr int SCRIPT_MAIN_MENU = 0;
constexpr int SCRIPT_NEW_TRIGGER = 1;
constexpr int SCRIPT_DEL_TRIGGER = 2;

#define OLC_SCRIPT_EDIT_MODE(d)    (OLC(d)->script_mode)    /* parse input mode */
#define OLC_SCRIPT(d)           (OLC(d)->script)    /* script editing   */
#define OLC_ITEM_TYPE(d)    (OLC(d)->item_type)    /* mob/obj/room     */

/* prototype exported functions from dg_olc.c */
extern void dg_olc_script_copy(struct descriptor_data *d);

extern void dg_script_menu(struct descriptor_data *d);

extern int dg_script_edit_parse(struct descriptor_data *d, char *arg);

extern void trigedit_string_cleanup(struct descriptor_data *d, int terminator);

extern void trigedit_save(struct descriptor_data *d);

extern int format_script(struct descriptor_data *d);

extern ACMD(do_oasis_trigedit);
