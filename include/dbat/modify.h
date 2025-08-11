//
// Created by basti on 10/21/2021.
//
#pragma once

#include "structs.h"


// functions
extern void smash_tilde(char *str);
extern void string_write(struct descriptor_data *d, char **writeto, size_t len, long mailto, void *data);
extern void string_write(struct descriptor_data *d, std::string *writeto, size_t len, long mailto, std::string backup);
extern void string_add(struct descriptor_data *d, char *str);
extern void std_string_add(struct descriptor_data *d, char *str);

// Helper functions for std::string editor (static functions don't need external declarations)

// commands
extern ACMD(do_skillset);
