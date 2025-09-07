
#pragma once
#include <vector>
#include <string>

struct CharacterPrototype;
struct ObjectPrototype;
struct Guild;
struct Shop;
struct DgScriptPrototype;

std::vector<std::string> getRaceNames();
std::vector<std::string> getSenseiNames();
std::vector<std::string> getFormNames();
std::vector<std::string> getSkillNames();
std::vector<std::string> getRoomFlagNames();
std::vector<std::string> getSectorTypeNames();
std::vector<std::string> getSizeNames();
std::vector<std::string> getPlayerFlagNames();
std::vector<std::string> getMobFlagNames();
std::vector<std::string> getPrefFlagNames();
std::vector<std::string> getAffectFlagNames();
std::vector<std::string> getItemTypeNames();
std::vector<std::string> getWearFlagNames();
std::vector<std::string> getItemFlagNames();
std::vector<std::string> getAdminFlagNames();
std::vector<std::string> getDirectionNames();
std::vector<std::string> getAttributeNames();
std::vector<std::string> getAttributeTrainNames();
std::vector<std::string> getAppearanceNames();
std::vector<std::string> getAlignNames();
std::vector<std::string> getMoneyNames();
std::vector<std::string> getVitalNames();
std::vector<std::string> getStatNames();
std::vector<std::string> getDimNames();
std::vector<std::string> getComStatNames();
std::vector<std::string> getShopFlagNames();
std::vector<std::string> getCharacterFlagNames();
std::vector<std::string> getZoneFlagNames();
std::vector<std::string> getWhereFlagNames();
std::vector<std::string> getSexNames();
std::vector<std::string> getMutationNames();
std::vector<std::string> getBioGenomeNames();
std::vector<std::string> getAndroidModelNames();

extern std::vector<CharacterPrototype *> collectNPCProtos(int start_vnum, int end_vnum);
extern std::vector<ObjectPrototype *> collectItemProtos(int start_vnum, int end_vnum);
extern std::vector<Guild *> collectGuilds(int start_vnum, int end_vnum);
extern std::vector<Shop *> collectShops(int start_vnum, int end_vnum);
extern std::vector<DgScriptPrototype *> collectTriggers(int start_vnum, int end_vnum);