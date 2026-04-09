#pragma once
#include "dbat/db/consts/types.h"
#include <string>

bool tech_handle_android_absorb(char_data *ch, char_data *vict);
bool tech_handle_zanzoken(char_data *ch, char_data *vict, const std::string& name);
void tech_handle_posmodifier(char_data *vict, int &pry, int &blk, int &dge, int &prob);
void tech_handle_fireshield(char_data *ch, char_data *vict, const std::string& part = "body");
bool tech_handle_targeting(char_data *ch, char *arg, char_data **vict, obj_data **obj);
bool tech_handle_charge(char_data *ch, char *arg, double minimum, double *attperc);
bool tech_handle_crashdown(char_data *ch, char_data *vict);
