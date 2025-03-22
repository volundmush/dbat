#pragma once
#include "structs.h"
#include "dbat/attack.h"

namespace trans {
    extern std::string getName(struct char_data *ch, Form form);
    extern std::string getAbbr(struct char_data *ch, Form form);
    extern std::string getExtra(struct char_data *ch, Form form);
    extern int getMaxGrade(struct char_data *ch, Form form);

    extern bool unlock(struct char_data *ch, Form form);

    extern double getModifier(struct char_data *ch, int location, int specific);
    extern double getModifierExact(struct char_data *ch, int location, int specific);

    extern double getStaminaDrain(struct char_data *ch, Form form, bool upkeep = false);

    extern void handleEchoTransform(struct char_data *ch, Form form);
    extern void handleEchoRevert(struct char_data *ch, Form form);

    extern void displayForms(struct char_data *ch);
    extern int64_t getRequiredPL(struct char_data* ch, Form trans);
    extern int getFormType(struct char_data* ch, Form trans);
    extern std::optional<Form> findFormFor(char_data* ch, const std::string& form);
    extern std::unordered_set<Form> getFormsFor(char_data* ch);

    extern bool blockRevertDisallowed(struct char_data *ch, Form form);

    extern std::optional<Form> findForm(struct char_data *ch, const std::string& arg);

    extern void onAttacked(char_data *ch, atk::Attack& incoming, Form form);
    extern void onAttack(char_data *ch, atk::Attack& outgoing, Form form);

    extern void transform(struct char_data *ch, Form form, int grade = 1);
    extern void revert(char_data* ch);

    extern void gamesys_transform(uint64_t heartPulse, double deltaTime);
    extern void gamesys_oozaru(uint64_t heartPulse, double deltaTime);

    extern std::optional<std::string> getAppearance(char_data *ch, Form form, Appearance type);
}