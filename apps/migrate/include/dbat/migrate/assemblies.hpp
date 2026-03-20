#pragma once

#include <vector>
#include <unordered_map>
#include "dbat/game/Typedefs.hpp"
#include <nlohmann/json_fwd.hpp>

struct Character;

typedef struct component_data COMPONENT;

struct component_data {
    bool bExtract;               
    bool bInRoom;                
    vnum lVnum;                  
};

struct assembly_data {
    vnum lVnum;                  
    unsigned char uchAssemblyType;
    std::vector<component_data> pComponents;
};

extern std::unordered_map<vnum, assembly_data> g_mAssemblyTable;

constexpr int ASSM_MAKE = 0;
constexpr int ASSM_BAKE = 1;
constexpr int ASSM_BREW = 2;
constexpr int ASSM_ASSEMBLE = 3;
constexpr int ASSM_CRAFT = 4;
constexpr int ASSM_FLETCH = 5;
constexpr int ASSM_KNIT = 6;
constexpr int ASSM_MIX = 7;
constexpr int ASSM_THATCH = 8;
constexpr int ASSM_WEAVE = 9;
constexpr int ASSM_FORGE = 10;

extern void assemblyListToChar(Character *pCharacter, int type = 0);

extern bool assemblyAddComponent(vnum lVnum, vnum lComponentVnum,
                                 bool bExtract, bool bInRoom);

extern bool assemblyCheckComponents(vnum lVnum, Character *pCharacter, int extract_yes);

extern bool assemblyCreate(vnum lVnum, int iAssembledType);

extern bool assemblyDestroy(vnum lVnum);

extern bool assemblyHasComponent(vnum lVnum, vnum lComponentVnum);

extern bool assemblyRemoveComponent(vnum lVnum, vnum lComponentVnum);

extern int assemblyGetType(vnum lVnum);

extern size_t assemblyCountComponents(vnum lVnum);

extern vnum assemblyFindAssembly(const char *pszAssemblyName);

extern vnum assemblyGetComponentVnum(assembly_data *pAssembly, size_t index);

extern assembly_data *assemblyGetAssemblyPtr(vnum lVnum);
