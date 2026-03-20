#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/assemblies.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/utils.hpp"
#include <nlohmann/json.hpp>

std::unordered_map<vnum, assembly_data> g_mAssemblyTable;

void assemblyListToChar(Character *pCharacter, int type)
{
    char szBuffer[MAX_STRING_LENGTH] = {'\0'};
    char szAssmType[MAX_INPUT_LENGTH] = {'\0'};
    long lRnum = 0;

    if (pCharacter == nullptr)
    {
        basic_mud_log("SYSERR: assemblyListAssembliesToChar(): nullptr 'pCharacter'.");
        return;
    }
    else if (g_mAssemblyTable.empty())
    {
        pCharacter->sendText("No assemblies exist.\r\n");
        return;
    }

    if (type == 0)
        pCharacter->sendText("The following assemblies exists:\r\n");
    else
        pCharacter->send_to("Only displaying [%s]\r\n", item_types[type]);

    for (const auto& [vnum, assembly] : g_mAssemblyTable)
    {
        if ((lRnum = real_object(assembly.lVnum)) < 0)
        {
            pCharacter->sendText("[-----] ***RESERVED***\r\n");
            basic_mud_log("SYSERR: assemblyListToChar(): Invalid vnum #%ld in assembly table.", assembly.lVnum);
        }
        else
        {
            if (type == 0 || type == static_cast<int>(obj_proto.at(lRnum)->type_flag))
            {
                sprinttype(assembly.uchAssemblyType, AssemblyTypes, szAssmType, sizeof(szAssmType));
                sprintf(szBuffer, "[%5d] %s (%s)\r\n", assembly.lVnum,
                        obj_proto.at(lRnum)->short_description.c_str(), szAssmType);
                pCharacter->sendText(szBuffer);

                if (GET_ADMLEVEL(pCharacter) > 0)
                {
                    for (const auto& comp : assembly.pComponents)
                    {
                        if ((lRnum = real_object(comp.lVnum)) < 0)
                        {
                            pCharacter->sendText(" -----: ***RESERVED***\r\n");
                            basic_mud_log("SYSERR: assemblyListToChar(): Invalid component vnum #%ld in assembly for vnum #%ld.",
                                          comp.lVnum, assembly.lVnum);
                        }
                        else
                        {
                            sprintf(szBuffer, " %5d: %-20.20s Extract=%-3.3s InRoom=%-3.3s\r\n",
                                    +comp.lVnum,
                                    obj_proto.at(lRnum)->short_description.c_str(),
                                    (comp.bExtract ? "Yes" : "No"),
                                    (comp.bInRoom ? "Yes" : "No"));
                            pCharacter->sendText(szBuffer);
                        }
                    }
                }
            }
        }
    }
}

bool assemblyAddComponent(vnum lVnum, vnum lComponentVnum, bool bExtract, bool bInRoom)
{
    auto it = g_mAssemblyTable.find(lVnum);

    if (it == g_mAssemblyTable.end())
    {
        basic_mud_log("SYSERR: assemblyAddComponent(): Invalid 'lVnum' #%ld.", lVnum);
        return false;
    }
    else if (real_object(lComponentVnum) <= NOTHING)
    {
        basic_mud_log("SYSERR: assemblyAddComponent(): Invalid 'lComponentVnum' #%ld.", lComponentVnum);
        return false;
    }

    it->second.pComponents.push_back({bExtract, bInRoom, lComponentVnum});
    return true;
}

bool assemblyCheckComponents(vnum lVnum, Character *pCharacter, int extract_yes)
{
    bool bOk = true;
    long lRnum = 0;
    Object **ppComponentObjects = nullptr;

    if (pCharacter == nullptr)
    {
        basic_mud_log("SYSERR: nullptr assemblyCheckComponents(): 'pCharacter'.");
        return false;
    }

    auto it = g_mAssemblyTable.find(lVnum);
    if (it == g_mAssemblyTable.end())
    {
        basic_mud_log("SYSERR: nullptr assemblyCheckComponents(): Invalid 'lVnum' #%ld.", lVnum);
        return false;
    }

    auto& assembly = it->second;
    if (assembly.pComponents.empty())
        return false;

    size_t numComponents = assembly.pComponents.size();
    CREATE(ppComponentObjects, Object *, numComponents);

    for (size_t i = 0; i < numComponents && bOk; i++)
    {
        if ((lRnum = real_object(assembly.pComponents[i].lVnum)) < 0)
            bOk = false;
        else
        {
            if (assembly.pComponents[i].bInRoom)
            {
                if ((ppComponentObjects[i] = pCharacter->location.searchObjects(lRnum)) == nullptr)
                    bOk = false;
                else
                {
                    ppComponentObjects[i]->clearLocation();
                }
            }
            else
            {
                if ((ppComponentObjects[i] = pCharacter->searchInventory(lRnum)) == nullptr)
                    bOk = false;
                else
                {
                    ppComponentObjects[i]->clearLocation();
                }
            }
        }
    }

    for (size_t i = 0; i < numComponents; i++)
    {
        if (ppComponentObjects[i] == nullptr)
            continue;

        if (assembly.pComponents[i].bExtract && bOk && extract_yes == true)
            extract_obj(ppComponentObjects[i]);
        else if (assembly.pComponents[i].bInRoom)
            ppComponentObjects[i]->moveToLocation(pCharacter);
        else
            pCharacter->addToInventory(ppComponentObjects[i]);
    }

    free(ppComponentObjects);
    return bOk;
}

bool assemblyCreate(vnum lVnum, int iAssembledType)
{
    if (lVnum < 0)
        return false;
    else if (iAssembledType < 0 || iAssembledType >= MAX_ASSM)
        return false;

    if (g_mAssemblyTable.contains(lVnum))
        return false;

    g_mAssemblyTable[lVnum] = {lVnum, static_cast<unsigned char>(iAssembledType), {}};
    return true;
}

bool assemblyDestroy(vnum lVnum)
{
    if (g_mAssemblyTable.empty() || !g_mAssemblyTable.contains(lVnum))
    {
        basic_mud_log("SYSERR: assemblyDestroy(): Invalid 'lVnum' #%ld.", lVnum);
        return false;
    }

    g_mAssemblyTable.erase(lVnum);
    return true;
}

bool assemblyHasComponent(vnum lVnum, vnum lComponentVnum)
{
    auto it = g_mAssemblyTable.find(lVnum);
    if (it == g_mAssemblyTable.end())
    {
        basic_mud_log("SYSERR: assemblyHasComponent(): Invalid 'lVnum' #%ld.", lVnum);
        return false;
    }

    for (const auto& comp : it->second.pComponents)
    {
        if (comp.lVnum == lComponentVnum)
            return true;
    }
    return false;
}

bool assemblyRemoveComponent(vnum lVnum, vnum lComponentVnum)
{
    auto it = g_mAssemblyTable.find(lVnum);
    if (it == g_mAssemblyTable.end())
    {
        basic_mud_log("SYSERR: assemblyRemoveComponent(): Invalid 'lVnum' #%ld.", lVnum);
        return false;
    }

    auto& components = it->second.pComponents;
    auto compIt = std::find_if(components.begin(), components.end(),
                               [lComponentVnum](const component_data& c) { return c.lVnum == lComponentVnum; });

    if (compIt == components.end())
    {
        basic_mud_log("SYSERR: assemblyRemoveComponent(): Vnum #%ld is not a component of assembled vnum #%ld.",
                      lComponentVnum, lVnum);
        return false;
    }

    components.erase(compIt);
    return true;
}

int assemblyGetType(vnum lVnum)
{
    auto it = g_mAssemblyTable.find(lVnum);
    if (it == g_mAssemblyTable.end())
    {
        basic_mud_log("SYSERR: assemblyGetType(): Invalid 'lVnum' #%ld.", lVnum);
        return -1;
    }

    return static_cast<int>(it->second.uchAssemblyType);
}

size_t assemblyCountComponents(vnum lVnum)
{
    auto it = g_mAssemblyTable.find(lVnum);
    if (it == g_mAssemblyTable.end())
    {
        basic_mud_log("SYSERR: assemblyCountComponents(): Invalid 'lVnum' #%ld.", lVnum);
        return 0;
    }

    return it->second.pComponents.size();
}

vnum assemblyFindAssembly(const char *pszAssemblyName)
{
    long lRnum = NOTHING;

    if (g_mAssemblyTable.empty())
        return -1;
    else if (pszAssemblyName == nullptr || *pszAssemblyName == '\0')
        return -1;

    for (const auto& [vnum, assembly] : g_mAssemblyTable)
    {
        if ((lRnum = real_object(assembly.lVnum)) < 0)
            basic_mud_log("SYSERR: assemblyFindAssembly(): Invalid vnum #%ld in assembly table.", assembly.lVnum);
        else if (isname(pszAssemblyName, obj_proto.at(lRnum)->name))
            return assembly.lVnum;
    }

    return -1;
}

vnum assemblyGetComponentVnum(assembly_data *pAssembly, size_t index)
{
    if (pAssembly == nullptr || index >= pAssembly->pComponents.size())
        return -1;
    return pAssembly->pComponents[index].lVnum;
}

assembly_data *assemblyGetAssemblyPtr(vnum lVnum)
{
    auto it = g_mAssemblyTable.find(lVnum);
    if (it == g_mAssemblyTable.end())
        return nullptr;
    return &it->second;
}

void free_assemblies()
{
    g_mAssemblyTable.clear();
}

void to_json(nlohmann::json& j, const component_data& c)
{
    j = nlohmann::json{
        {"bExtract", c.bExtract},
        {"bInRoom", c.bInRoom},
        {"lVnum", c.lVnum}
    };
}

void from_json(const nlohmann::json& j, component_data& c)
{
    c.bExtract = j.at("bExtract").get<bool>();
    c.bInRoom = j.at("bInRoom").get<bool>();
    c.lVnum = j.at("lVnum").get<vnum>();
}

void to_json(nlohmann::json& j, const assembly_data& a)
{
    j = nlohmann::json{
        {"lVnum", a.lVnum},
        {"uchAssemblyType", a.uchAssemblyType},
        {"pComponents", a.pComponents}
    };
}

void from_json(const nlohmann::json& j, assembly_data& a)
{
    a.lVnum = j.at("lVnum").get<vnum>();
    a.uchAssemblyType = j.at("uchAssemblyType").get<unsigned char>();
    if (j.contains("pComponents"))
    {
        j.at("pComponents").get_to(a.pComponents);
    }
}
