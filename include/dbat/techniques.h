#pragma once

#include "sysdep.h"
#include "typestubs.h"

extern bool tech_handle_android_absorb(Character *ch, Character *vict);

extern bool tech_handle_zanzoken(Character *ch, Character *vict, const std::string &name);

extern void tech_handle_posmodifier(Character *vict, int &pry, int &blk, int &dge, int &prob);

extern void tech_handle_fireshield(Character *ch, Character *vict, const std::string &part = "body");

extern bool tech_handle_targeting(Character *ch, char *arg, Character **vict, Object **obj);

extern bool tech_handle_charge(Character *ch, char *arg, double minimum, double *attperc);

extern void tech_handle_crashdown(Character *ch, Character *vict);
