#pragma once
#include "structs.h"

namespace trans {
    extern std::string getName(BaseCharacter *ch, FormID form);
    extern std::string getAbbr(BaseCharacter *ch, FormID form);

    extern bool unlock(BaseCharacter *ch, FormID form);

    extern double getModifier(BaseCharacter *ch, int location, int specific = 0);

    extern double getStaminaDrain(BaseCharacter *ch, FormID form, bool upkeep = false);

    extern std::optional<int> getAppearanceMod(BaseCharacter *ch, FormID form, CharAppearance mode);

    extern void handleEchoTransform(BaseCharacter *ch, FormID form);
    extern void handleEchoRevert(BaseCharacter *ch, FormID form);

    extern void displayForms(BaseCharacter *ch);
    extern int64_t getRequiredPL(BaseCharacter* ch, FormID trans);
    extern std::optional<FormID> findFormFor(BaseCharacter* ch, const std::string& form);
    extern std::set<FormID> getFormsFor(BaseCharacter* ch);

    extern bool blockRevertDisallowed(BaseCharacter *ch, FormID form);

    extern std::optional<FormID> findForm(BaseCharacter *ch, const std::string& arg);

    extern void gamesys_transform(uint64_t heartPulse, double deltaTime);
    extern void gamesys_oozaru(uint64_t heartPulse, double deltaTime);

}