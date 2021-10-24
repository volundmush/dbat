#include "races.h"

#include <utility>
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "comm.h"

const char *race_names[NUM_RACES+1] = {
  "human",
  "saiyan",
  "icer",
  "konatsu",
  "namekian",
  "mutant",
  "kanassan",
  "halfbreed",
  "bioandroid",
  "android",
  "demon",
  "majin",
  "kai",
  "truffle",
  "hoshijin",
  "animal",
  "saiba",
  "serpent",
  "ogre",
  "yardratian",
  "arlian",
  "dragon",
  "mechanical",
  "spirit",
  "\n"
};

const char *race_abbrevs[NUM_RACES+1] = {
  "Hum",
  "Sai",
  "Ice",
  "Kon",
  "Nam",
  "Mut",
  "Kan",
  "H-B",
  "Bio",
  "And",
  "Dem",
  "Maj",
  "Kai",
  "Tru",
  "Hos",
  "Ict",
  "Sab",
  "Ser",
  "Trl",
  "Dra",
  "Arl",
  "Mnd",
  "Mec",
  "Spi",
  "\n"
};

const char *pc_race_types[NUM_RACES+1] = {
  "Human",
  "Saiyan",
  "Icer",
  "Konatsu",
  "Namekian",
  "Mutant",
  "Kanassan",
  "Halfbreed",
  "Bioandroid",
  "Android",
  "Demon",
  "Majin",
  "Kai",
  "Truffle",
  "Hoshijin",
  "animal",
  "Saiba",
  "Serpent",
  "Ogre",
  "Yardratian",
  "Arlian",
  "Dragon",
  "mechanical",
  "Spirit",
  "\n"
};

const char *d_race_types[NUM_RACES+1] = {
  "A Disguised Human",
  "A Disguised Saiyan",
  "A Disguised Icer",
  "A Disguised Konatsu",
  "A Disguised Namekian",
  "A Disguised Mutant",
  "A Disguised Kanassan",
  "A Disguised Halfbreed",
  "A Disguised Bioandroid",
  "A Disguised Android",
  "A Disguised Demon",
  "A Disguised Majin",
  "A Disguised Kai",
  "A Disguised Truffle",
  "A Disguised Hoshijin",
  "A Disguised Animal",
  "Saiba",
  "Serpent",
  "Ogre",
  "Yardratian",
  "A Disguised Arlian",
  "Dragon",
  "mechanical",
  "Spirit",
  "\n"
};

#define Y   TRUE
#define N   FALSE

/* Original Race/Gender Breakout */
const int race_ok_gender[NUM_SEX][NUM_RACES] = {
/*        H, S, I, K, N, M, Ka, HB, B, A, D, Ma, Kai, TR, G, I, O, S, T, M, Ar, L, W, F */
/* N */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N, N, N, N, Y, N, N, N },
/* M */ { Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N, N, N, N, Y, N, N, N },
/* F */ { Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N, N, N, N, Y, N, N, N }

};

const char *race_display[NUM_RACES] = {
  "@B1@W) @cHuman\r\n",
  "@B2@W) @cSaiyan\r\n",
  "@B3@W) @cIcer\r\n",
  "@B4@W) @cKonatsu\r\n",
  "@B5@W) @cNamekian\r\n",
  "@B6@W) @cMutant\r\n",
  "@B7@W) @cKanassan\r\n",
  "@B8@W) @cHalf Breed\r\n",
  "@B9@W) @cBio-Android\r\n",
  "@B10@W) @cAndroid\r\n",
  "@B11@W) @cDemon\r\n",
  "@B12@W) @cMajin\r\n",
  "@B13@W) @cKai\r\n",
  "@B14@W) @cTruffle\r\n",
  "@B15@W) @cHoshijin\r\n",
  "@B16@W) @YArlian\r\n",
  "@B17@W) @GAnimal\r\n",
  "@B18@W) @MSaiba\r\n",
  "@B19@W) @BSerpent\r\n",
  "@B20@W) @ROgre\r\n",
  "@B21@W) @CYardratian\r\n",
  "@B22@W) @GLizardfolk\r\n",
  "@B23@W) @GMechanical\r\n",
  "@B24@W) @MSpirit\r\n",
};

/*
 * The code to interpret a race letter (used in interpreter.c when a
 * new character is selecting a race).
 */
int parse_race(struct char_data *ch, int arg)
{
  int race = RACE_UNDEFINED;

  switch (arg) {
  case 1:  race = RACE_HUMAN      ; break;
  case 2:
   if (ch->desc->rpp >= 60) {
   race = RACE_SAIYAN;
   //ch->desc->rpp -= 60;
   userWrite(ch->desc, 0, 0, 0, "index");
   } else if (ch->desc->rbank >= 60) {
    race = RACE_SAIYAN;
    //ch->desc->rbank -= 60;
    userWrite(ch->desc, 0, 0, 0, "index");
   }
   else {
    race = RACE_UNDEFINED;
   }
   break;
  case 3:  race = RACE_ICER       ; break;
  case 4:  race = RACE_KONATSU    ; break;
  case 5:  race = RACE_NAMEK      ; break;
  case 6:  race = RACE_MUTANT     ; break;
  case 7:  race = RACE_KANASSAN   ; break;
  case 8:  race = RACE_HALFBREED  ; break;
  case 9:  
   if (ch->desc->rpp >= 35) {
    race = RACE_BIO;
    //ch->desc->rpp -= 35;
    userWrite(ch->desc, 0, 0, 0, "index");
   } else if (ch->desc->rbank >= 35) {
    race = RACE_BIO;
    //ch->desc->rbank -= 35;
    userWrite(ch->desc, 0, 0, 0, "index");
   }
   else {
    race = RACE_UNDEFINED;
   }
   break;
  case 10: race = RACE_ANDROID    ; break;
  case 11: race = RACE_DEMON      ; break;
  case 12:
   if (ch->desc->rpp >= 55) {
    race = RACE_MAJIN;
    //ch->desc->rpp -= 55;
    userWrite(ch->desc, 0, 0, 0, "index");
   } else if (ch->desc->rbank >= 55) {
    race = RACE_MAJIN;
    //ch->desc->rbank -= 55;
    userWrite(ch->desc, 0, 0, 0, "index");
   }
   else {
    race = RACE_UNDEFINED;
   }
   break;
  case 13: race = RACE_KAI        ; break;
  case 14: race = RACE_TRUFFLE    ; break;
  case 15: race = RACE_GOBLIN     ; 
   if (ch->desc->rpp >= 30) {
    race = RACE_GOBLIN;
    //ch->desc->rpp -= 30;
    userWrite(ch->desc, 0, 0, 0, "index");
   } else if (ch->desc->rbank >= 30) {
    race = RACE_GOBLIN;
    //ch->desc->rbank -= 30;
    userWrite(ch->desc, 0, 0, 0, "index");
   }
   else {
    race = RACE_UNDEFINED;
   }
   break;
  case 16: race = RACE_KOBOLD     ; break;
  case 17: race = RACE_ANIMAL     ; break;
  case 18: race = RACE_ORC        ; break;
  case 19: race = RACE_SNAKE      ; break;
  case 20: race = RACE_TROLL      ; break;
  case 21: race = RACE_MINOTAUR   ; break;
  case 22: race = RACE_LIZARDFOLK ; break;
  case 23: race = RACE_WARHOST    ; break;
  case 24: race = RACE_FAERIE     ; break;
  default: race = RACE_UNDEFINED  ; break;
  }
  if (race >= 0 && race < NUM_RACES)
    if (!race_ok_gender[(int)GET_SEX(ch)][race])
      race = RACE_UNDEFINED;

  return (race);
}

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
int racial_ability_mods[][6] = {
/*                      Str,Con,Int,Wis,Dex,Cha */
/* RACE_HUMAN       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_SAIYAN      */ {  0, -2,  0,  0,  2,  0 },
/* RACE_ICER        */ { -2,  2,  0,  0,  0,  0 },
/* RACE_KONATSU     */ {  0,  2,  0,  0,  0, -2 },
/* RACE_NAMEK       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_MUTANT      */ { -2,  0,  0,  0,  2,  0 },
/* RACE_KANASSAN    */ {  0, -2,  2,  0,  2,  2 },
/* RACE_HALFBREED   */ {  2,  0, -2,  0,  0, -2 },
/* RACE_BIO         */ {  0,  0,  0,  0,  0,  0 },
/* RACE_ANDROID     */ {  0,  0,  0,  0,  0,  0 },
/* RACE_DEMON       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_MAJIN       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_KAI         */ {  0,  0,  0,  0,  0,  0 },
/* RACE_TRUFFLE     */ { 14,  8, -4,  0, -2, -4 },
/* RACE_GOBLIN      */ { -2,  0,  0,  0,  2, -2 },
/* RACE_ANIMAL      */ {  0,  0,  0,  0,  0,  0 },
/* RACE_ORC         */ {  4,  0, -2, -2,  0, -2 },
/* RACE_SNAKE       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_TROLL       */ { 12, 12, -4, -2,  4, -4 },
/* RACE_MINOTAUR    */ {  8,  4, -4,  0,  0, -2 },
/* RACE_KOBOLD      */ { -4, -2,  0,  0,  2,  0 },
/* RACE_LIZARDFOLK  */ {  0,  0,  0,  0,  0,  0 },
/* RACE_WARHOST     */ {  0,  0,  0,  0,  0,  0 },
/* RACE_FAERIE      */ {  0,  0,  0,  0,  0,  0 },
{ 0, 0, 0, 0, 0}
};

void racial_ability_modifiers(struct char_data *ch)
{
  int chrace = 0;
  if (GET_RACE(ch) >= NUM_RACES || GET_RACE(ch) < 0) {
    log("SYSERR: Unknown race %d in racial_ability_modifiers", GET_RACE(ch));
  } else {
    chrace = GET_RACE(ch);
  }

  /*ch->real_abils.str += racial_ability_mods[chrace][0];
  ch->real_abils.con += racial_ability_mods[chrace][1];
  ch->real_abils.intel += racial_ability_mods[chrace][2];
  ch->real_abils.wis += racial_ability_mods[chrace][3];
  ch->real_abils.dex += racial_ability_mods[chrace][4];*/
}


/* Converted into metric units: cm and kg; SRD has english units. */
struct {
  int height[NUM_SEX];	/* cm */
  int heightdie;	/* 2d(heightdie) added to height */
  int weight[NUM_SEX];	/* kg */
  int weightfac;	/* added height * weightfac/100 added to weight */
} hw_info[NUM_RACES] = {
/* RACE_HUMAN      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_SAIYAN     */ { {140, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ICER       */ { {100, 111, 95}, 10, {17, 18, 16}, 18},
/* RACE_KONATSU    */ { {121, 124, 109}, 20, {52, 59, 45}, 125},
/* RACE_NAMEK      */ { {137, 140, 135}, 20, {40, 45, 36}, 89},
/* RACE_MUTANT     */ { {141, 150, 140}, 10, {46, 54, 39}, 89},
/* RACE_KANASSAN   */ { {135, 135, 135}, 15, {37, 39, 36}, 63},
/* RACE_HALFBREED  */ { {141, 147, 135}, 30, {59, 68, 50}, 125},
/* RACE_BIO        */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ANDROID    */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_DEMON      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MAJIN      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_KAI        */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_TRUFFLE    */ { {40, 50, 45}, 16, {16, 24, 9}, 8},
/* RACE_GOBLIN     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ANIMAL     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ORC        */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_SNAKE      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_TROLL      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MINOTAUR   */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_KOBOLD     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_LIZARDFOLK */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_WARHOST    */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_FAERIE     */ { {141, 147, 135}, 26, {46, 54, 39}, 89}
};


void set_height_and_weight_by_race(struct char_data *ch)
{
  int race, sex, mod;

  if (!IS_NPC(ch)) {
   return;
  }

  race = GET_RACE(ch);
  sex = GET_SEX(ch);
  if (sex < SEX_NEUTRAL || sex >= NUM_SEX) {
    log("Invalid gender in set_height_and_weight_by_race: %d", sex);
    sex = SEX_NEUTRAL;
  }
  if (race <= RACE_UNDEFINED || race >= NUM_RACES) {
    log("Invalid gender in set_height_and_weight_by_race: %d", GET_SEX(ch));
    race = RACE_UNDEFINED + 1; /* first defined race */
  }

  mod = dice(2, hw_info[race].heightdie);
  GET_HEIGHT(ch) = hw_info[race].height[sex] + mod;
  mod *= hw_info[race].weightfac;
  mod /= 100;
  GET_WEIGHT(ch) = hw_info[race].weight[sex] + mod;
}


int invalid_race(struct char_data *ch, struct obj_data *obj)
{
  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_SAIYAN) && IS_SAIYAN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ICER) && IS_ICER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_KONATSU) && IS_KONATSU(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_HUMAN) && !IS_HUMAN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_ICER) && !IS_ICER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_SAIYAN) && !IS_SAIYAN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_KONATSU) && !IS_KONATSU(ch))
    return TRUE;

  return FALSE;
}


const int race_def_sizetable[NUM_RACES + 1] =
{
/* HUMAN */	SIZE_MEDIUM,
/* SAIYAN */	SIZE_MEDIUM,
/* ICER */	SIZE_MEDIUM,
/* KONATSU */	SIZE_MEDIUM,
/* NAMEK */	SIZE_MEDIUM,
/* MUTANT */	SIZE_MEDIUM,
/* KANASSAN */	SIZE_MEDIUM,
/* HALFBREED */	SIZE_MEDIUM,
/* BIO */	SIZE_MEDIUM,
/* ANDROID */	SIZE_MEDIUM,
/* DEMON */	SIZE_MEDIUM,
/* MAJIN */	SIZE_MEDIUM,
/* KAI */	SIZE_MEDIUM,
/* TRUFFLE */	SIZE_SMALL,
/* GOBLIN */	SIZE_MEDIUM,
/* ANIMAL */	SIZE_FINE,
/* ORC */	SIZE_LARGE,
/* SNAKE */	SIZE_MEDIUM,
/* TROLL */	SIZE_LARGE,
/* MINOTAUR */	SIZE_MEDIUM,
/* Arlian */	SIZE_MEDIUM,
/* LIZARDFOLK */SIZE_MEDIUM,
/* WARHOST */	SIZE_MEDIUM,
/* FAERIE */	SIZE_TINY
};


int get_size(struct char_data *ch)
{
  int racenum;

  if (ch->size != SIZE_UNDEFINED)
    return ch->size;
  else {
    racenum = GET_RACE(ch);
    if (racenum < 0 || racenum >= NUM_RACES)
      return SIZE_MEDIUM;
    return (ch->size = race_def_sizetable[racenum]);
  }
}


const int size_bonus_table[NUM_SIZES] = {
/* XTINY */	8,
/* TINY */	4,
/* XSMALL */	2,
/* SMALL */	1,
/* MEDIUM */	0,
/* LARGE */	-1,
/* HUGE */	-2,
/* GIGANTIC */	-4,
/* COLOSSAL */	-8
};


int get_size_bonus(int sz)
{
  if (sz < 0 || sz >= NUM_SIZES)
    sz = SIZE_MEDIUM;
  return size_bonus_table[sz];
}


int wield_type(int chsize, const struct obj_data *weap)
{
  if (GET_OBJ_TYPE(weap) != ITEM_WEAPON) {
    return OBJ_FLAGGED(weap, ITEM_2H) ? WIELD_TWOHAND : WIELD_ONEHAND;
  } else if (chsize > GET_OBJ_SIZE(weap)) {
    return WIELD_LIGHT;
  } else if (chsize == GET_OBJ_SIZE(weap)) {
    return WIELD_ONEHAND;
  } else if (chsize == GET_OBJ_SIZE(weap) - 1) {
    return WIELD_TWOHAND;
  } else if (chsize < GET_OBJ_SIZE(weap) - 1) {
    return WIELD_NONE; /* It's just too big for you! */
  } else {
    log("unknown size vector in wield_type: chsize=%d, weapsize=%d", chsize, GET_OBJ_SIZE(weap));
    return WIELD_NONE;
  }
}

int race_bodyparts[NUM_RACES][NUM_WEARS] = {
                     /* 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22 */
                     /* U, F, F, N, N, B, H, L, F, H, A, U, A, W, W, W, W, W, B, E, E, W, M */
/* RACE_HUMAN       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_SAIYAN      */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_ICER        */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_KONATSU     */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_NAMEK       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_MUTANT      */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_KANASSAN    */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_HALFBREED   */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_BIO         */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_ANDROID     */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_DEMON       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_MAJIN       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_KAI         */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_TRUFFLE     */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_HOSHIJIN    */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_ANIMAL      */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 },
/* RACE_ORC         */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 },
/* RACE_SNAKE       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 },
/* RACE_TROLL       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 },
/* RACE_MINOTAUR    */ {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 },
/* RACE_ARLIAN      */ {0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_LIZARDFOLK  */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 },
/* RACE_WARHOST     */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* RACE_FAERIE      */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

void racial_body_parts(struct char_data *ch) {
  int i;

  for (i=1; i<NUM_WEARS; i++) {
    if (race_bodyparts[GET_RACE(ch)][i]) {
      SET_BIT_AR(BODY_PARTS(ch), i);
    } else {
      if (BODY_FLAGGED(ch, i))
        REMOVE_BIT_AR(BODY_PARTS(ch), i);
    }
  }
}

namespace dbat { namespace race {

        Race::Race(race_id rid, std::string name, std::string abbr, int size, bool pc) {
            this->r_id = rid;
            this->name = name;
            this->lower_name = name;
            race_abbr = std::move(abbr);
            disg = "A Disguised " + name;
            // just generate lower name from the name.
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::toupper);
            pc_check = pc;
            race_size = size;
        }

        race_id Race::getID() const {
            return r_id;
        }

        const std::string &Race::getName() const {
            return name;
        }

        const std::string &Race::getNameLower() const {
            return lower_name;
        }

        const std::string &Race::getAbbr() const {
            return race_abbr;
        }

        int Race::getSize() const {
            return race_size;
        }

        bool Race::isPcOk() const {
            return pc_check;
        }

        bool Race::isValidSex(int sex_id) const {
            return true;
        }

        bool Race::raceCanTransform() const {
            return true;
        }

        bool Race::raceCanRevert() const {
            return true;
        }

        bool Race::checkCanTransform(char_data *ch) const {
            if (GET_KAIOKEN(ch) > 0) {
                send_to_char(ch, "You are in kaioken right now and can't transform!\r\n");
                return false;
            }
            if (GET_SUPPRESS(ch) > 0) {
                send_to_char(ch, "You are suppressing right now and can't transform!\r\n");
                return false;
            }
            if (GET_CLONES(ch) > 0) {
                send_to_char(ch, "You can't concentrate on transforming while your body is split into multiple forms!\r\n");
                return false;
            }
            return true;
        }



        void Race::handle_transform(char_data *ch, int64_t add, double mult, double drain) const {
            if (!ch)
                return;
            if (IS_NPC(ch))
                return;
            else {
                long double cur, max, dapercent = GET_LIFEPERC(ch);
                /* Handle Pl */
                cur = (long double)(GET_HIT(ch));
                max = (long double)(GET_MAX_HIT(ch));
                GET_MAX_HIT(ch) = (GET_BASE_PL(ch) + add) * mult;
                if (android_can(ch) == 1)
                    GET_MAX_HIT(ch) += 50000000;
                else if (android_can(ch) == 2)
                    GET_MAX_HIT(ch) += 20000000;

                if ((GET_HIT(ch) + (add * (cur / max))) * mult <= gear_pl(ch)) {
                    GET_HIT(ch) = (GET_HIT(ch) + (add * (cur / max))) * mult;
                }
                else if ((GET_HIT(ch) + (add * (cur / max))) * mult > gear_pl(ch)) {
                    GET_HIT(ch) = gear_pl(ch);
                }

                /* Handle Ki */
                cur = (long double)(GET_MANA(ch));
                max = (long double)(GET_MAX_MANA(ch));
                GET_MAX_MANA(ch) = (GET_BASE_KI(ch) + add) * mult;
                if (android_can(ch) == 1)
                    GET_MAX_MANA(ch) += 50000000;
                else if (android_can(ch) == 2)
                    GET_MAX_MANA(ch) += 20000000;

                if ((GET_MANA(ch) + (add * (cur / max))) * mult <= GET_MAX_MANA(ch)) {
                    GET_MANA(ch) = (GET_MANA(ch) + (add * (cur / max))) * mult;
                }
                else if ((GET_MANA(ch) + (add * (cur / max))) * mult > GET_MAX_MANA(ch)) {
                    GET_MANA(ch) = GET_MAX_MANA(ch);
                }

                /* Handle St */
                GET_MOVE(ch) -= GET_MOVE(ch) * drain;
                cur = (long double)(GET_MOVE(ch));
                max = (long double)(GET_MAX_MOVE(ch));
                GET_MAX_MOVE(ch) = (GET_BASE_ST(ch) + add) * mult;
                if (android_can(ch) == 1)
                    GET_MAX_MOVE(ch) += 50000000;
                else if (android_can(ch) == 2)
                    GET_MAX_MOVE(ch) += 20000000;

                if ((GET_MOVE(ch) + (add * (cur / max))) * mult <= GET_MAX_MOVE(ch)) {
                    GET_MOVE(ch) = (GET_MOVE(ch) + (add * (cur / max))) * mult;
                }
                else if ((GET_MOVE(ch) + (add * (cur / max))) * mult > GET_MAX_MOVE(ch)) {
                    GET_MOVE(ch) = GET_MAX_MOVE(ch);
                }

                if (!IS_ANDROID(ch)) {
                    /* Handle Lifeforce */
                    /*R: Tried changing initial GET_LIFEFORCE to GET_LIFEMAX
                    I: You are setting lifemax, which is set dynamically by what KI and Stamina's maxes are.
                    This is updated all the time, so it isn't going to do anything for more than a fraction
                    of a second. You need to adjust GET_LIFEFORCE to be the same percentage of the new max as
                    it was the old. To do this you find out the old */
                    /* Example: percent = ((long double)(GET_LIFEMAX(ch)) / 100) * (long double)(GET_LIFEFORCE(ch))


                    This will give you the percent (in decimal form) of lifemax. Do this prior to the transformation
                    change at the start of handle_transformation. Then here at the end you do:
                    GET_LIFEFORCE(ch) = GET_LIFEMAX(ch) * percent;
                    The percent will remain the same, but based off the new detransformed lifeforce max. Make sure the
                    variable, percent, hasn't already been used. Declare it or something else for your code.*/

                    GET_LIFEFORCE(ch) = GET_LIFEMAX(ch) * dapercent;
                    if (GET_LIFEFORCE(ch) > GET_LIFEMAX(ch)) {
                        GET_LIFEFORCE(ch) = GET_LIFEMAX(ch);
                    }
                }


            }
        }

        void Race::display_forms(char_data *ch) const {
            send_to_char(ch, "You do not have a transformation.\r\n");
        }

        void Race::displayTransReq(char_data *ch) const {

            switch(GET_TRANSCLASS(ch)) {
                case 1:
                    send_to_char(ch, "\r\n@RYou have @rterrible@R transformation BPL Requirements.@n\r\n");
                    break;
                case 2:
                    send_to_char(ch, "\r\n@CYou have @caverage@C transformation BPL Requirements.@n\r\n");
                    break;
                case 3:
                    send_to_char(ch, "\r\n@GYou have @gGREAT@G transformation BPL Requirements.@n\r\n");
                    break;
            }
        }

        std::map<race_id, Race*> race_map;

        void load_races() {
            if(!race_map.empty())
                return;
            race_map[race_id::human] = new Human(race_id::human, "Human", "Hum", SIZE_MEDIUM, true);
            race_map[race_id::saiyan] = new Saiyan(race_id::saiyan, "Saiyan", "Sai", SIZE_MEDIUM, true);
            race_map[race_id::icer] = new Icer(race_id::icer, "Icer", "Ice",  SIZE_MEDIUM, true);
            race_map[race_id::konatsu] = new Konatsu(race_id::konatsu, "Konatsu", "kon", SIZE_MEDIUM, true);
            race_map[race_id::namekian] = new Namekian(race_id::namekian, "Namekian", "Nam", SIZE_MEDIUM, true);
            race_map[race_id::mutant] = new Mutant(race_id::mutant, "Mutant", "Mut", SIZE_MEDIUM, true);
            race_map[race_id::kanassan] = new Kanassan(race_id::kanassan, "Kanassan", "Kan", SIZE_MEDIUM, true);
            race_map[race_id::halfbreed] = new Halfbreed(race_id::halfbreed, "Halfbreed", "H-B", SIZE_MEDIUM, true);
            race_map[race_id::bio] = new BioAndroid(race_id::bio, "BioAndroid", "Bio", SIZE_MEDIUM, true);
            race_map[race_id::android] = new Android(race_id::android, "Android", "And", SIZE_MEDIUM, true);
            race_map[race_id::demon] = new Demon(race_id::demon, "Demon", "Dem", SIZE_MEDIUM, true);
            race_map[race_id::majin] = new Majin(race_id::majin, "Majin", "Maj", SIZE_MEDIUM, true);
            race_map[race_id::kai] = new Kai(race_id::kai, "Kai", "Kai", SIZE_MEDIUM, true);
            race_map[race_id::truffle] = new Truffle(race_id::truffle, "Truffle", "Tru", SIZE_SMALL, true);
            race_map[race_id::hoshijin] = new Hoshijin(race_id::hoshijin, "Hoshijin", "Hos", SIZE_MEDIUM, true);
            race_map[race_id::animal] = new Animal(race_id::animal, "Animal", "Ict", SIZE_FINE, true);
            race_map[race_id::saiba] = new Saiba(race_id::saiba, "Saiba", "Sab", SIZE_LARGE, false);
            race_map[race_id::serpent] = new Serpent(race_id::serpent, "Serpent", "Ser", SIZE_MEDIUM, false);
            race_map[race_id::ogre] = new Ogre(race_id::ogre, "Ogre", "Ogr", SIZE_LARGE, false);
            race_map[race_id::yardratian] = new Yardratian(race_id::yardratian, "Yardratian", "Yar", SIZE_MEDIUM, false);
            race_map[race_id::arlian] = new Arlian(race_id::arlian, "Arlian", "Arl", SIZE_MEDIUM, true);
            race_map[race_id::dragon] = new Dragon(race_id::dragon, "Dragon", "Drg", SIZE_MEDIUM, false);
            race_map[race_id::mechanical] = new Mechanical(race_id::mechanical, "Mechanical", "Mec", SIZE_MEDIUM, false);
            race_map[race_id::spirit] = new Spirit(race_id::spirit, "Spirit", "Spi", SIZE_TINY, false);

        }


        // all race-specific stuff goes below here.

        /* Human */
        void Human::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YSuper @CHuman@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YSuper @CHuman @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YSuper @CHuman @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YSuper @CHuman @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@YSuper @CHuman @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Saiyan */
        void Saiyan::display_forms(char_data *ch) const {
            if(PLR_FLAGGED(ch, PLR_LSSJ)) {
                send_to_char(ch, "                @YSuper @CSaiyan@n\r\n");
                send_to_char(ch, "@b-------------------------------------------------@n\r\n");
                send_to_char(ch, "@YSuper @CSaiyan @WFirst   @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
                send_to_char(ch, "@YLegendary @CSuper Saiyan @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
                send_to_char(ch, "@b-------------------------------------------------@n\r\n");
            } else {
                send_to_char(ch, "              @YSuper @CSaiyan@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YSuper @CSaiyan @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
                send_to_char(ch, "@YSuper @CSaiyan @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
                send_to_char(ch, "@YSuper @CSaiyan @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
                send_to_char(ch, "@YSuper @CSaiyan @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
            }
        }

        bool Saiyan::checkCanTransform(char_data *ch) const {
            if (PLR_FLAGGED(ch, PLR_OOZARU)) {
                send_to_char(ch, "You are the great Oozaru right now and can't transform!\r\n");
                return false;
            }
            return Race::checkCanTransform(ch);
        }

        /* Icer */
        void Icer::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YTransform@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YTransform @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YTransform @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YTransform @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@YTransform @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Konatsu */
        void Konatsu::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YShadow@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YShadow @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YShadow @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YShadow @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Namekian */
        void Namekian::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YSuper @CNamek@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YSuper @CNamek @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YSuper @CNamek @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YSuper @CNamek @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@YSuper @CNamek @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Mutant */
        void Mutant::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YMutate@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YMutate @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YMutate @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YMutate @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Kanassan */
        bool Kanassan::raceCanTransform() const {
            return false;
        }

        /* Halfbreed */
        void Halfbreed::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YSuper @CSaiyan@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YSuper @CSaiyan @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YSuper @CSaiyan @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YSuper @CSaiyan @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* BioAndroid */
        void BioAndroid::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YPerfection@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YMature        @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YSemi-Perfect  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YPerfect       @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@YSuper Perfect @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Android */
        void Android::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YUpgrade@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@Y1.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@Y2.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@Y3.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@Y4.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
            send_to_char(ch, "@Y5.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 5) * 0.75) ? add_commas(trans_req(ch, 5)) : "??????????");
            send_to_char(ch, "@Y6.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 6) * 0.75) ? add_commas(trans_req(ch, 6)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Demon */
        bool Demon::raceCanTransform() const {
            return false;
        }

        /* Majin */
        void Majin::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YMorph@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YMorph @WAffinity @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YMorph @WSuper    @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YMorph @WTrue     @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Kai */
        void Kai::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YMystic@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YMystic @WFirst     @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YMystic @WSecond    @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YMystic @WThird     @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

        /* Truffle */
        void Truffle::display_forms(char_data *ch) const {
            send_to_char(ch, "              @YAscend@n\r\n");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
            send_to_char(ch, "@YAscend @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
            send_to_char(ch, "@YAscend @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
            send_to_char(ch, "@YAscend @WThird @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
            send_to_char(ch, "@b------------------------------------------------@n\r\n");
        }

}}

