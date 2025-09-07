#include <map>
#include "dbat/API.h"
#include "dbat/Enum.h"

#include "dbat/const/Race.h"
#include "dbat/const/Sensei.h"
#include "dbat/const/Form.h"
#include "dbat/const/Skill.h"
#include "dbat/const/RoomFlag.h"
#include "dbat/const/SectorType.h"
#include "dbat/const/Size.h"
#include "dbat/const/PlayerFlag.h"
#include "dbat/const/MobFlag.h"
#include "dbat/const/CharacterFlag.h"
#include "dbat/const/PrefFlag.h"
#include "dbat/const/AffectFlag.h"
#include "dbat/const/Sex.h"
#include "dbat/const/ItemType.h"
#include "dbat/const/WearFlag.h"
#include "dbat/const/ItemFlag.h"
#include "dbat/const/AdminFlag.h"
#include "dbat/const/Direction.h"
#include "dbat/const/CharacterProperties.h"
#include "dbat/const/ZoneFlag.h"
#include "dbat/const/WhereFlag.h"
#include "dbat/const/Mutation.h"
#include "dbat/const/Appearance.h"
#include "dbat/const/ShopFlag.h"

#include "dbat/races.h"

#include "dbat/CharacterPrototype.h"
#include "dbat/ObjectPrototype.h"
#include "dbat/Shop.h"
#include "dbat/Guild.h"
#include "dbat/DgScriptPrototype.h"



std::vector<std::string> getRaceNames() {
        return getEnumNameList<Race>();
}

std::vector<std::string> getSenseiNames() {
        return getEnumNameList<Sensei>();
}

std::vector<std::string> getFormNames() {
        return getEnumNameList<Form>();
}

std::vector<std::string> getSkillNames() {
        return getEnumNameList<Skill>();
}

std::vector<std::string> getRoomFlagNames() {
        return getEnumNameList<RoomFlag>();
}

std::vector<std::string> getSectorTypeNames() {
        return getEnumNameList<SectorType>();
}

std::vector<std::string> getSizeNames() {
        return getEnumNameList<Size>();
}

std::vector<std::string> getPlayerFlagNames() {
        return getEnumNameList<PlayerFlag>();
}

std::vector<std::string> getMobFlagNames() {
        return getEnumNameList<MobFlag>();
}

std::vector<std::string> getPrefFlagNames() {
        return getEnumNameList<PrefFlag>();
}

std::vector<std::string> getAffectFlagNames() {
        return getEnumNameList<AffectFlag>();
}

std::vector<std::string> getItemTypeNames() {
        return getEnumNameList<ItemType>();
}

std::vector<std::string> getWearFlagNames() {
        return getEnumNameList<WearFlag>();
}

std::vector<std::string> getItemFlagNames() {
        return getEnumNameList<ItemFlag>();
}

std::vector<std::string> getAdminFlagNames() {
        return getEnumNameList<AdminFlag>();
}

std::vector<std::string> getDirectionNames() {
        return getEnumNameList<Direction>();
}

std::vector<std::string> getAttributeNames() {
        return getEnumNameList<CharAttribute>();
}

std::vector<std::string> getAttributeTrainNames() {
        return getEnumNameList<CharTrain>();
}

std::vector<std::string> getAppearanceNames() {
        return getEnumNameList<Appearance>();
}

std::vector<std::string> getAlignNames() {
        return getEnumNameList<CharAlign>();
}

std::vector<std::string> getMoneyNames() {
        return getEnumNameList<CharMoney>();
}

std::vector<std::string> getVitalNames() {
        return getEnumNameList<CharVital>();
}

std::vector<std::string> getStatNames() {
        return getEnumNameList<CharStat>();
}

std::vector<std::string> getDimNames() {
        return getEnumNameList<CharDim>();
}

std::vector<std::string> getComStatNames() {
        return getEnumNameList<ComStat>();
}

std::vector<std::string> getShopFlagNames() {
        return getEnumNameList<ShopFlag>();
}

std::vector<std::string> getCharacterFlagNames() {
        return getEnumNameList<CharacterFlag>();
}

std::vector<std::string> getZoneFlagNames() {
        return getEnumNameList<ZoneFlag>();
}

std::vector<std::string> getWhereFlagNames() {
        return getEnumNameList<WhereFlag>();
}

std::vector<std::string> getSexNames() {
        return getEnumNameList<Sex>();
}

std::vector<std::string> getMutationNames() {
        return getEnumNameList<Mutation>();
}

std::vector<std::string> getAndroidModelNames() {
        return getEnumNameList<AndroidModel>();
}

std::vector<std::string> getBioGenomeNames() {
        std::vector<std::string> out;
        for (auto val : magic_enum::enum_values<Race>()) {
                if(race::isValidGenome(val)) {
                        out.emplace_back(magic_enum::enum_name(val));
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