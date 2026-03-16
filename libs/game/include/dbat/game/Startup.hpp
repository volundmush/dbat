#pragma once
#include <ctime>

extern void check_start_rooms();
extern void boot_db_textfiles();
extern void boot_db_spellfeats();
extern void boot_db_world();
extern void boot_db_help();
extern void boot_db_mail();
extern void boot_db_socials();
extern void boot_db_commands();
extern void boot_db_specials();
extern void boot_db_sort();
extern void boot_db_boards();
extern void boot_db_shadow();

extern void boot_db_spacemap();
extern void boot_db_sort();

extern void boot_db_new();

namespace dbat::init {
    void init();
    void init_locale();
    void init_database();
    void init_zones();
}