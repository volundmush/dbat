#pragma once
#include <unordered_set>
#include "const/Max.hpp"
#include "Typedefs.hpp"
struct Character;
struct Object;

// variables
extern const room_vnum freeres[NUM_ALIGNS];

// functions
extern void log_imm_action(const char *messg, ...);

extern void hint_system(Character *ch, int num);

extern std::unordered_set<Object *> dball_count(Character *ch);

extern void log_custom(struct descriptor_data *d, Object *obj);

extern void wishSYS(uint64_t heartPulse, double deltaTime);

extern void bring_to_cap(Character *ch);

extern void base_update(uint64_t heartPulse, double deltaTime);

extern void load_shadow_dragons();

extern void situpProgress(Character *ch);
extern void meditateProgress(Character *ch);
extern void pushupProgress(Character *ch);
extern void trainProgress(Character *ch);

// commands
