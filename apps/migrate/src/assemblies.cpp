#include "dbat/migrate/assemblies.hpp"

#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Descriptor.hpp"

#include "dbat/game/handler.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/utils.hpp"

std::unordered_map<vnum, assembly_data> g_mAssemblyTable;

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
