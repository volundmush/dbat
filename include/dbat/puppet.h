#include "net.h"

namespace net {
    class PuppetParser : public ConnectionParser {
    public:
        PuppetParser(const std::shared_ptr<Connection>& co, char_data *c);
        void parse(const std::string &txt) override;
        void start() override;
        bool canCopyover() override {return true;};
        std::string getName() override;
        nlohmann::json serialize() override;
        void deserialize(const nlohmann::json& j) override;
    protected:
        char_data *ch;
    };
}