#pragma once
#include <unordered_set>
#include "attack.hpp"

#include "const/Form.hpp"
#include "const/Appearance.hpp"

namespace trans
{
    extern std::string getName(Character *ch, Form form);
    extern std::string getAbbr(Character *ch, Form form);
    extern std::string getExtra(Character *ch, Form form);
    extern int getMaxGrade(Character *ch, Form form);

    extern bool unlock(Character *ch, Form form);

    extern double getModifier(Character *ch, int location, int specific);
    extern double getModifierExact(Character *ch, int location, int specific);

    extern double getStaminaDrain(Character *ch, Form form, bool upkeep = false);

    extern void handleEchoTransform(Character *ch, Form form);
    extern void handleEchoRevert(Character *ch, Form form);

    extern void displayForms(Character *ch);
    extern int64_t getRequiredPL(Character *ch, Form trans);
    extern int getFormType(Character *ch, Form trans);
    extern std::optional<Form> findFormFor(Character *ch, const std::string &form);
    extern std::unordered_set<Form> getFormsFor(Character *ch);

    extern bool blockRevertDisallowed(Character *ch, Form form);

    extern std::optional<Form> findForm(Character *ch, const std::string &arg);

    extern void onAttacked(Character *ch, atk::Attack &incoming, Form form);
    extern void onAttack(Character *ch, atk::Attack &outgoing, Form form);

    extern void transform(Character *ch, Form form, int grade = 1);
    extern void revert(Character *ch);

    extern void gamesys_transform(uint64_t heartPulse, double deltaTime);
    extern void gamesys_oozaru(uint64_t heartPulse, double deltaTime);

    extern std::optional<std::string> getAppearance(Character *ch, Form form, Appearance type);
}