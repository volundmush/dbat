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

std::vector<std::string> getSubRaceNames() {
        return getEnumNameList<SubRace>();
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
