#pragma once
#include "net.h"
#include "structs.h"

namespace net {
    class CharacterMenu : public ConnectionParser {
    public:
        CharacterMenu(const std::shared_ptr<Connection>& co, char_data *c);
        void parse(const std::string &txt) override;
        void start() override;
        bool canCopyover() override {return true;};
        std::string getName() override;
        nlohmann::json serialize() override;
        void deserialize(const nlohmann::json& j) override;
    protected:
        struct char_data *ch;
        int state;
    };
}