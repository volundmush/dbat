//
// Created by volund on 11/4/21.
//

#ifndef CIRCLE_TECHNIQUES_H
#define CIRCLE_TECHNIQUES_H

#include "sysdep.h"
#include "typestubs.h"

bool tech_handle_android_absorb(char_data *ch, char_data *vict);
bool tech_handle_zanzoken(char_data *ch, char_data *vict, const std::string& name);
void tech_handle_posmodifier(char_data *vict, int &pry, int &blk, int &dge, int &prob);
void tech_handle_fireshield(char_data *ch, char_data *vict, const std::string& part = "body");
bool tech_handle_targeting(char_data *ch, char *arg, char_data **vict, obj_data **obj);
bool tech_handle_charge(char_data *ch, char *arg, double minimum, double *attperc);

#endif //CIRCLE_TECHNIQUES_H
