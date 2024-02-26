#pragma once
#include "structs.h"

namespace trans {
    extern std::string getName(Character *ch, FormID form);
    extern std::string getAbbr(Character *ch, FormID form);

    extern bool unlock(Character *ch, FormID form);

    extern double getModifier(Character *ch, int location, int specific = 0);

    extern double getStaminaDrain(Character *ch, FormID form, bool upkeep = false);

    extern std::optional<int> getAppearanceMod(Character *ch, FormID form, CharAppearance mode);

    extern void handleEchoTransform(Character *ch, FormID form);
    extern void handleEchoRevert(Character *ch, FormID form);

    extern void displayForms(Character *ch);
    extern int64_t getRequiredPL(Character* ch, FormID trans);
    extern std::optional<FormID> findFormFor(Character* ch, const std::string& form);
    extern std::set<FormID> getFormsFor(Character* ch);

    extern bool blockRevertDisallowed(Character *ch, FormID form);

    extern std::optional<FormID> findForm(Character *ch, const std::string& arg);

    extern void gamesys_transform(uint64_t heartPulse, double deltaTime);
    extern void gamesys_oozaru(uint64_t heartPulse, double deltaTime);

}