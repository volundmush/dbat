#pragma once
#include "net.h"
#include "structs.h"

namespace net {
    class CharacterMenu : public ConnectionParser {
    public:
        CharacterMenu(std::shared_ptr<Connection>& co, BaseCharacter *c);
        void parse(const std::string &txt) override;
        void start() override;
    protected:
        BaseCharacter *ch;
        int state;
    };
}