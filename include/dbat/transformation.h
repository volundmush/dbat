#pragma once
#include "structs.h"

namespace trans {

    struct Transformation {
        virtual int getID() = 0;
        virtual std::string getName() = 0;
        virtual int getSlot() = 0;
        virtual double getBonus(struct char_data *ch, int location, int specific);
        virtual std::optional<int> overrideAppearance(struct char_data *ch, int mode);
        virtual std::string descriptionLine(struct char_data *ch);
        virtual void onUpdate(double deltaTime);
        virtual void onLoseBodyPart(struct char_data *ch, int limb);
        virtual bool isSkill() {return false;}; // for Kaioken, DarkMeta, and maybe other things.
        virtual bool isGodly() { return false;};
        virtual bool isDemonic() { return false;};
        virtual void onTransform(struct char_data *ch, bool voluntary);
        virtual void onRevert(struct char_data *ch, bool voluntary);
        virtual bool canTransform(struct char_data *ch);
        virtual bool canRevert(struct char_data *ch);
        virtual void announceTransform(struct char_data *ch, bool toRoom);
        virtual void announceRevert(struct char_data *ch, bool toRoom);
    };

    struct Base : public Transformation {
        int getID() override { return 0;};
        std::string getName() override {return "base";}
        int getSlot() override { return -1;}; // Any slot
    };

    // SLOT 1 Transformations:


    // SLOT 2 Transformations:
    struct Oozaru : public Transformation {
        int getID() override { return 1000;}
        std::string getName() override { return "Oozaru";};
    };



    // SLOT 3 Transformations

}