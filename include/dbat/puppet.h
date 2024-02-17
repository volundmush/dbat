#include "net.h"

namespace net {
    class PuppetParser : public ConnectionParser {
    public:
        PuppetParser(std::shared_ptr<Connection>& co, BaseCharacter *c);
        void parse(const std::string &txt) override;
        void start() override;
    protected:
        BaseCharacter *ch;
    };
}