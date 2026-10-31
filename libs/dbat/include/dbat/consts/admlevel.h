#pragma once
/*
 * ADMLVL_IMPL should always be the HIGHEST possible admin level, and
 * ADMLVL_IMMORT should always be the LOWEST immortal level.
 */
constexpr int ADMLVL_NONE    = 0;
constexpr int ADMLVL_IMMORT  = 1;
constexpr int ADMLVL_BUILDER = 2;
constexpr int ADMLVL_GOD     = 3;
constexpr int ADMLVL_VICE    = 4;
constexpr int ADMLVL_GRGOD   = 5;
constexpr int ADMLVL_IMPL    = 6;

extern const char *admin_level_names[ADMLVL_IMPL + 2];