#include "dbat/DragonBall.h"


int dballtime = 0;              /* used by dragonball load system*/
int SHENRON = false;            /* Shenron has been summoned     */
int DRAGONR = 0;                /* Room Shenron has been summoned to */
int DRAGONZ = 0;                /* Zone Shenron has been summoned to */
int WISH[2] = {0, 0};           /* Keeps track of wishes granted */
int DRAGONC = 0;                /* Keeps count of Shenron's remaining time */
Character *EDRAGON = nullptr;      /* This is Shenron when he is loaded */

std::unordered_set<obj_vnum> dbVnums = {20, 21, 22, 23, 24, 25, 26};

int SELFISHMETER = 0;
int SHADOW_DRAGON1 = -1;
int SHADOW_DRAGON2 = -1;
int SHADOW_DRAGON3 = -1;
int SHADOW_DRAGON4 = -1;
int SHADOW_DRAGON5 = -1;
int SHADOW_DRAGON6 = -1;
int SHADOW_DRAGON7 = -1;

int DBALL_HUNTER1 = -1;
int DBALL_HUNTER2 = -1;
int DBALL_HUNTER3 = -1;
int DBALL_HUNTER4 = -1;

int DBALL_HUNTER1_VNUM = 88;
int DBALL_HUNTER2_VNUM = 89;
int DBALL_HUNTER3_VNUM = 0;
int DBALL_HUNTER4_VNUM = 0;