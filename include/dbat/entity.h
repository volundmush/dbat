#include "dbat/structs.h"

namespace flags {
    bool check(entt::entity ent, FlagType type, int flag);
    void set(entt::entity ent, FlagType type, int flag, bool value = true);
    void clear(entt::entity ent, FlagType type, int flag);
    bool flip(entt::entity ent, FlagType type, int flag);
    std::vector<std::string> getNames(entt::entity ent, FlagType type);
}