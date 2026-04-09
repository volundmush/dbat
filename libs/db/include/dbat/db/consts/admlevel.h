#pragma once
/*
 * ADMLVL_IMPL should always be the HIGHEST possible admin level, and
 * ADMLVL_IMMORT should always be the LOWEST immortal level.
 */
#define ADMLVL_NONE		0
#define ADMLVL_IMMORT		1
#define ADMLVL_BUILDER          2
#define ADMLVL_GOD		3
#define ADMLVL_VICE             4
#define ADMLVL_GRGOD		5
#define ADMLVL_IMPL		6

extern const char *admin_level_names[ADMLVL_IMPL+2];