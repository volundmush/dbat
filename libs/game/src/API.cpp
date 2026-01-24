#include <map>
#include "dbat/game/API.hpp"
#include "volcano/util/Enum.hpp"

#include "dbat/game/const/Race.hpp"
#include "dbat/game/const/Sensei.hpp"
#include "dbat/game/const/Form.hpp"
#include "dbat/game/const/Skill.hpp"
#include "dbat/game/const/RoomFlag.hpp"
#include "dbat/game/const/SectorType.hpp"
#include "dbat/game/const/Size.hpp"
#include "dbat/game/const/PlayerFlag.hpp"
#include "dbat/game/const/MobFlag.hpp"
#include "dbat/game/const/CharacterFlag.hpp"
#include "dbat/game/const/PrefFlag.hpp"
#include "dbat/game/const/AffectFlag.hpp"
#include "dbat/game/const/Sex.hpp"
#include "dbat/game/const/ItemType.hpp"
#include "dbat/game/const/WearFlag.hpp"
#include "dbat/game/const/ItemFlag.hpp"
#include "dbat/game/const/AdminFlag.hpp"
#include "dbat/game/const/Direction.hpp"
#include "dbat/game/const/CharacterProperties.hpp"
#include "dbat/game/const/ZoneFlag.hpp"
#include "dbat/game/const/WhereFlag.hpp"
#include "dbat/game/const/Mutation.hpp"
#include "dbat/game/const/Appearance.hpp"
#include "dbat/game/const/ShopFlag.hpp"

#include "dbat/game/races.hpp"

#include "dbat/game/CharacterPrototype.hpp"
#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/DgScriptPrototype.hpp"



std::vector<std::string> getRaceNames() {
        return volcano::util::getEnumNameList<Race>();
}

std::vector<std::string> getSenseiNames() {
        return volcano::util::getEnumNameList<Sensei>();
}

std::vector<std::string> getFormNames() {
        return volcano::util::getEnumNameList<Form>();
}

std::vector<std::string> getSkillNames() {
        return volcano::util::getEnumNameList<Skill>();
}

std::vector<std::string> getRoomFlagNames() {
        return volcano::util::getEnumNameList<RoomFlag>();
}

std::vector<std::string> getSectorTypeNames() {
        return volcano::util::getEnumNameList<SectorType>();
}

std::vector<std::string> getSizeNames() {
        return volcano::util::getEnumNameList<Size>();
}

std::vector<std::string> getPlayerFlagNames() {
        return volcano::util::getEnumNameList<PlayerFlag>();
}

std::vector<std::string> getMobFlagNames() {
        return volcano::util::getEnumNameList<MobFlag>();
}

std::vector<std::string> getPrefFlagNames() {
        return volcano::util::getEnumNameList<PrefFlag>();
}

std::vector<std::string> getAffectFlagNames() {
        return volcano::util::getEnumNameList<AffectFlag>();
}

std::vector<std::string> getItemTypeNames() {
        return volcano::util::getEnumNameList<ItemType>();
}

std::vector<std::string> getWearFlagNames() {
        return volcano::util::getEnumNameList<WearFlag>();
}

std::vector<std::string> getItemFlagNames() {
        return volcano::util::getEnumNameList<ItemFlag>();
}

std::vector<std::string> getAdminFlagNames() {
        return volcano::util::getEnumNameList<AdminFlag>();
}

std::vector<std::string> getDirectionNames() {
        return volcano::util::getEnumNameList<Direction>();
}

std::vector<std::string> getAttributeNames() {
        return volcano::util::getEnumNameList<CharAttribute>();
}

std::vector<std::string> getAttributeTrainNames() {
        return volcano::util::getEnumNameList<CharTrain>();
}

std::vector<std::string> getAppearanceNames() {
        return volcano::util::getEnumNameList<Appearance>();
}

std::vector<std::string> getAlignNames() {
        return volcano::util::getEnumNameList<CharAlign>();
}

std::vector<std::string> getMoneyNames() {
        return volcano::util::getEnumNameList<CharMoney>();
}

std::vector<std::string> getVitalNames() {
        return volcano::util::getEnumNameList<CharVital>();
}

std::vector<std::string> getStatNames() {
        return volcano::util::getEnumNameList<CharStat>();
}

std::vector<std::string> getDimNames() {
        return volcano::util::getEnumNameList<CharDim>();
}

std::vector<std::string> getComStatNames() {
        return volcano::util::getEnumNameList<ComStat>();
}

std::vector<std::string> getShopFlagNames() {
        return volcano::util::getEnumNameList<ShopFlag>();
}

std::vector<std::string> getCharacterFlagNames() {
        return volcano::util::getEnumNameList<CharacterFlag>();
}

std::vector<std::string> getZoneFlagNames() {
        return volcano::util::getEnumNameList<ZoneFlag>();
}

std::vector<std::string> getWhereFlagNames() {
        return volcano::util::getEnumNameList<WhereFlag>();
}

std::vector<std::string> getSexNames() {
        return volcano::util::getEnumNameList<Sex>();
}

std::vector<std::string> getMutationNames() {
        return volcano::util::getEnumNameList<Mutation>();
}

std::vector<std::string> getAndroidModelNames() {
        return volcano::util::getEnumNameList<AndroidModel>();
}

std::vector<std::string> getBioGenomeNames() {
        std::vector<std::string> out;
        for (const auto &[val, name] : enchantum::entries<Race>) {
                if(race::isValidGenome(val)) {
                        out.emplace_back(enchantum::to_string(val));
                }
        }
        
        return out;
}

template<typename T>
std::vector<T*> collectObjectsInRange(int start_vnum, int end_vnum, const std::map<int, std::shared_ptr<T>>& object_map) {
    std::vector<T*> result;
    
    for (int vnum = start_vnum; vnum <= end_vnum; ++vnum) {
        auto it = object_map.find(vnum);
        if (it != object_map.end()) {
            result.push_back(const_cast<T*>(it->second.get()));
        }
    }
    
    return result;
}

std::vector<CharacterPrototype*> collectNPCProtos(int start_vnum, int end_vnum) {
    return collectObjectsInRange(start_vnum, end_vnum, mob_proto);
}

std::vector<ObjectPrototype*> collectItemProtos(int start_vnum, int end_vnum) {
    return collectObjectsInRange(start_vnum, end_vnum, obj_proto);
}

std::vector<Guild*> collectGuilds(int start_vnum, int end_vnum) {
    return collectObjectsInRange(start_vnum, end_vnum, guild_index);
}

std::vector<Shop*> collectShops(int start_vnum, int end_vnum) {
    return collectObjectsInRange(start_vnum, end_vnum, shop_index);
}

std::vector<DgScriptPrototype*> collectTriggers(int start_vnum, int end_vnum) {
    return collectObjectsInRange(start_vnum, end_vnum, trig_index);
}