#pragma once

#include "sysdep.h"
#include "typestubs.h"

extern bool tech_handle_android_absorb(BaseCharacter *ch, BaseCharacter *vict);

extern bool tech_handle_zanzoken(BaseCharacter *ch, BaseCharacter *vict, const std::string &name);

extern void tech_handle_posmodifier(BaseCharacter *vict, int &pry, int &blk, int &dge, int &prob);

extern void tech_handle_fireshield(BaseCharacter *ch, BaseCharacter *vict, const std::string &part = "body");

extern bool tech_handle_targeting(BaseCharacter *ch, char *arg, BaseCharacter **vict, Object **obj);

extern bool tech_handle_charge(BaseCharacter *ch, char *arg, double minimum, double *attperc);

extern void tech_handle_crashdown(BaseCharacter *ch, BaseCharacter *vict);
