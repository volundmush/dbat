#include "dbat/structs.h"

void deserializeEntity(entt::entity ent, const nlohmann::json& j);
nlohmann::json serializeEntity(entt::entity ent);

void deserializeEntityRelations(entt::entity ent, const nlohmann::json& j);
nlohmann::json serializeEntityRelations(entt::entity ent);

namespace flags {
    bool check(entt::entity ent, FlagType type, int flag);
    void set(entt::entity ent, FlagType type, int flag, bool value = true);
    void clear(entt::entity ent, FlagType type, int flag);
    bool flip(entt::entity ent, FlagType type, int flag);
    std::vector<std::string> getNames(entt::entity ent, FlagType type);
}

namespace check {
    bool isPC(entt::entity ent);
    bool isNPC(entt::entity ent);

    // NOTE: all functions that return an empty optional, return empty if it's OK.
    // the optional string contains the reason why it's NOT OK, otherwise.

    // Whether u can access this unit's inventory.
    std::optional<std::string> checkAllowInventoryAcesss(entt::entity ent, entt::entity accessor);

    // whether this unit will allow u to take/give/drop it. give/put/drop are identical.
    std::optional<std::string> checkIsGettable(entt::entity ent, entt::entity getter);
    std::optional<std::string> checkIsGivable(entt::entity ent, entt::entity giver);

    // if this can store target in its inventory. This includes a room accepting items.
    // Things to check might be weight, item capacity, whether it's appropriate for the space, etc.
    std::optional<std::string> checkCanStore(entt::entity ent, entt::entity target);

    // Check to see whether giver can give target to ent.
    std::optional<std::string> checkAllowReceive(entt::entity ent, entt::entity giver, entt::entity target);

    // Whether ent can equip target in location.
    std::optional<std::string> checkAllowEquip(entt::entity ent, entt::entity target, int location);
    std::optional<std::string> checkAllowRemove(entt::entity ent);

}

namespace text {
    std::string get(entt::entity ent, const std::string& key, std::optional<std::string> placeholder = std::nullopt);
    void set(entt::entity ent, const std::string& key, const std::string& value);
    void clear(entt::entity ent, const std::string& key);
}

namespace vis {
    bool isInvisible(entt::entity ent);
    bool isHidden(entt::entity ent);
    bool isAdminInvisible(entt::entity ent);
    bool isProvidingLight(entt::entity ent);

    bool canSee(entt::entity viewer, entt::entity target);
    bool canSeeInvisible(entt::entity ent);
    bool canSeeHidden(entt::entity ent);
    bool canSeeAdminInvisible(entt::entity ent);
    bool canSeeInDark(entt::entity ent);

    bool isInsideNormallyDark(entt::entity ent, entt::entity viewer);
    bool isInsideDark(entt::entity ent, entt::entity viewer);

}

namespace find {
    entt::entity locationAncestorFlag(entt::entity ent, FlagType type, int flag);

    Object* object(entt::entity ent, const std::function<bool(Object*)> &func, bool working);

    std::set<Object*> gatherObjects(entt::entity ent, const std::function<bool(Object*)> &func, bool working);
    entt::entity holderType(entt::entity ent, int type);
}

int64_t getUID(entt::entity ent);
EntityFamily getFamily(entt::entity ent);
double getEnvVar(entt::entity env, EnvVar v);

namespace contents {
    std::vector<entt::entity> get(entt::entity ent);
    std::vector<entt::entity> get(GameEntity* ent);

    template<typename T>
    std::vector<T*> get(entt::entity ent) {
        std::vector<T*> result;
        for(auto e : get(ent)) {
            if(auto ptr = reg.try_get<T>(e)) {
                result.push_back(ptr);
            }
        }
        return result;
    }

    std::vector<GameEntity*> getContents(entt::entity ent);
    std::vector<GameEntity*> getContents(GameEntity* ent);

    std::vector<Object*> getInventory(entt::entity ent);
    std::vector<Object*> getInventory(GameEntity* ent);

    std::map<int, Object*> getEquipment(entt::entity ent);
    std::map<int, Object*> getEquipment(GameEntity* ent);

    std::vector<Character*> getPeople(entt::entity ent);
    std::vector<Character*> getPeople(GameEntity* ent);

    std::vector<Room*> getRooms(entt::entity ent);
    std::vector<Room*> getRooms(GameEntity* ent);

    std::map<int, entt::entity> getExits(entt::entity ent);
    std::map<int, entt::entity> getExits(GameEntity* ent);

    std::vector<GameEntity*> getNeighbors(entt::entity ent, bool visible = true);
    std::vector<GameEntity*> getNeighbors(GameEntity* ent, bool visible = true);

    void addTo(entt::entity ent, const Destination &dest);
    void addTo(GameEntity* ent, const Destination &dest);

    void removeFrom(entt::entity ent);
    void removeFrom(GameEntity* ent);

    void handleRemove(entt::entity ent, entt::entity mover);

    void handleAdd(entt::entity ent, entt::entity mover);

    void updateCoordinates(entt::entity ent, entt::entity mover, std::optional<Coordinates> previous = std::nullopt);
    void updateCoordinates(GameEntity* ent, std::optional<Coordinates> previous = std::nullopt);
}



namespace render {
    // The race and sex that ent appears to be, to viewer. This accounts for mimic.
    std::pair<RaceID, int> getApparentRaceSex(entt::entity ent, entt::entity viewer);

    // Helpful methods for getting pronouns and possessives.
    std::string hisHer(int sex, bool capitalized = false);
    std::string heShe(int sex, bool capitalized = false);
    std::string hisHer(int sex, bool capitalized = false);
    std::string himHer(int sex, bool capitalized = false);
    std::string hisHers(int sex, bool capitalized = false);
    std::string maFe(int sex, bool capitalized = false);

    // gets the apparent race name for ent to viewer.
    std::string apparentRaceName(entt::entity ent, entt::entity viewer, bool capitalized = false);

    // the search keywords viewer can use to find ent.
    std::vector<std::string> keywords(entt::entity ent, entt::entity viewer);

    // the personalized name viewer sees for ent.
    std::string displayName(entt::entity ent, entt::entity viewer);

    std::string inventory(entt::entity ent, entt::entity viewer);

    std::string equipment(entt::entity ent, entt::entity viewer, bool showEmpty = false);

    std::string diagnostics(entt::entity ent, entt::entity viewer);

    // renders the internal view. if ent is a room for instance, this is a room appearance
    // with exits and so on.
    std::string inside(entt::entity ent, entt::entity viewer);

    // A prefix of IDs, VN, Triggers, and other debug info for admin.
    // usually used by roomLine and contentLine.
    std::string listPrefix(entt::entity ent, entt::entity viewer);

    // A single line that is shown to represent ent to viewer when listed in a room/location.
    std::string roomLine(entt::entity ent, entt::entity viewer);

    // A single line that is shown to represent ent to viewer when listed in inventory, equipment, etc.
    std::string contentLine(entt::entity ent, entt::entity viewer);

    std::string modifiers(entt::entity ent, entt::entity viewer);

    // A series of lines shown 'under' the main list line detailing special status.
    // about the ent. This might be health status, buffs, whether food is partially eaten, etc.
    std::vector<std::string> statusLines(entt::entity ent, entt::entity viewer);

    // helper methods which will use listPrefix, room/contentLine and statusLines
    std::vector<std::string> fullRoomBlock(entt::entity ent, entt::entity viewer);
    std::vector<std::string> fullContentBlock(entt::entity ent, entt::entity viewer);
}

namespace send {
    void text(entt::entity ent, const std::string& txt);
    void event(entt::entity ent, const Event& event);
    void line(entt::entity ent, const std::string& txt);
    void textContents(entt::entity ent, const std::string& txt);
    void lineContents(entt::entity ent, const std::string& txt);

    template<typename... Args>
    void text(entt::entity ent, fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            text(ent, msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in send::text: %s", e.what());
        }
        
    }

    template<typename... Args>
    void lineContents(entt::entity ent, fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            lineContents(ent, msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in send::lineContents: %s", e.what());
        }
    }

    template<typename... Args>
    void textContents(entt::entity ent, fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            textContents(ent, msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in send::textContents: %s", e.what());
        }
    }

    template<typename... Args>
    void line(entt::entity ent, fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            line(ent, msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in send::line: %s", e.what());
        }
    }

    template<typename... Args>
    void printf(entt::entity ent, fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::sprintf(format, std::forward<Args>(args)...);
            text(ent, msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in send::printf: %s", e.what());
        }
    }

    template<typename... Args>
    void printfContents(entt::entity ent, fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::sprintf(format, std::forward<Args>(args)...);
            textContents(ent, msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in send::printfContents: %s", e.what());
        }
    }

}

namespace movement {
    std::map<int, Destination> getDestinations(entt::entity ent, entt::entity viewer);
    std::optional<Destination> getDestination(entt::entity ent, entt::entity viewer, int direction);
    bool moveInDirection(entt::entity ent, int direction, bool need_specials_check = true);
    bool doSimpleMove(entt::entity ent, int direction, bool need_specials_check = true);

    std::optional<Destination> getLaunchDestinationFor(entt::entity ent, entt::entity mover);
    std::vector<std::pair<std::string, Destination>> getLandingSpotsFor(entt::entity ent, entt::entity mover);

    bool checkPostEnter(entt::entity ent, entt::entity mover, const Location& loc, const Destination& dest);
    bool checkCanReachDestination(entt::entity ent, entt::entity mover, const Destination& dest);
    bool checkCanLeave(entt::entity ent, entt::entity mover, const Destination& dest, bool need_specials_check = true);


}