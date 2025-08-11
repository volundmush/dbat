/* ******************************************************************** *
 * FILE        : assemblies.c                  Copyright (C) 1999 Geoff Davis
*
 * USAGE: Implementation for assembly engine.                          *
 * -------------------------------------------------------------------- *
 * 1999 MAY 07 gdavis/azrael@laker.net Initial implementation.         *
 * ******************************************************************** */
#include "dbat/assemblies.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/constants.h"

/* Local global variables. */
long g_lNumAssemblies = 0;
ASSEMBLY *g_pAssemblyTable = nullptr;

void assemblyListToChar(struct char_data *pCharacter, int type) {
    char szBuffer[MAX_STRING_LENGTH] = {'\0'};
    char szAssmType[MAX_INPUT_LENGTH] = {'\0'};
    long i = 0;                  // Outer iterator.
    long j = 0;                  // Inner iterator.
    long lRnum = 0;              // Object rnum for obj_proto indexing.

    if (pCharacter == nullptr) {
        basic_mud_log("SYSERR: assemblyListAssembliesToChar(): nullptr 'pCharacter'.");
        return;
    } else if (g_pAssemblyTable == nullptr) {
                pCharacter->sendText("No assemblies exist.\r\n");
        return;
    }

    /* Send out a "header" of sorts. */
    if(type == 0)
                pCharacter->sendText("The following assemblies exists:\r\n");
    else
                pCharacter->send_to("Only displaying [%s]\r\n", item_types[type]);

    for (i = 0; i < g_lNumAssemblies; i++) {
        if ((lRnum = real_object(g_pAssemblyTable[i].lVnum)) < 0) {
                        pCharacter->sendText("[-----] ***RESERVED***\r\n");
            basic_mud_log("SYSERR: assemblyListToChar(): Invalid vnum #%ld in assembly table.", g_pAssemblyTable[i].lVnum);
        } else {
            if(type == 0 || type == static_cast<int>(obj_proto.at(lRnum).type_flag)) {
                sprinttype(g_pAssemblyTable[i].uchAssemblyType, AssemblyTypes, szAssmType, sizeof(szAssmType));
                sprintf(szBuffer, "[%5ld] %s (%s)\r\n", g_pAssemblyTable[i].lVnum,
                        obj_proto.at(lRnum).short_description, szAssmType);
                                pCharacter->sendText(szBuffer);

                if(GET_ADMLEVEL(pCharacter) > 0) {
                    for (j = 0; j < g_pAssemblyTable[i].lNumComponents; j++) {
                        if ((lRnum = real_object(g_pAssemblyTable[i].pComponents[j].lVnum)) < 0) {
                                                        pCharacter->sendText(" -----: ***RESERVED***\r\n");
                            basic_mud_log("SYSERR: assemblyListToChar(): Invalid component vnum #%ld in assembly for vnum #%ld.",
                                g_pAssemblyTable[i].pComponents[j].lVnum, g_pAssemblyTable[i].lVnum);
                        } else {
                            sprintf(szBuffer, " %5ld: %-20.20s Extract=%-3.3s InRoom=%-3.3s\r\n",
                                    +g_pAssemblyTable[i].pComponents[j].lVnum,
                                    obj_proto.at(lRnum).short_description,
                                    (g_pAssemblyTable[i].pComponents[j].bExtract ? "Yes" : "No"),
                                    (g_pAssemblyTable[i].pComponents[j].bInRoom ? "Yes" : "No"));
                                                        pCharacter->sendText(szBuffer);
                        }
                    }
                }
            }
        }
    }
}

bool assemblyAddComponent(long lVnum, long lComponentVnum, bool bExtract, bool bInRoom) {
    ASSEMBLY *pAssembly = nullptr;

    if ((pAssembly = assemblyGetAssemblyPtr(lVnum)) == nullptr) {
        basic_mud_log("SYSERR: assemblyAddComponent(): Invalid 'lVnum' #%ld.", lVnum);
        return (false);
    }
    else if( real_object( lComponentVnum ) <= NOTHING )
    {
        basic_mud_log("SYSERR: assemblyAddComponent(): Invalid 'lComponentVnum' #%ld.",
            lComponentVnum);
        return (false);
    }
    /* Removed as of 1.02.29 release */
    /* else if( assemblyHasComponent( lVnum, lComponentVnum ) )
    {
      log( "SYSERR: assemblyAddComponent(): Assembly for vnum #%ld already "
        "has component vnum #%ld.", lVnum, lComponentVnum );
      return (FALSE);
    } */

    /* Create a new component table with room for one more entry. */
    if (pAssembly->pComponents == nullptr)
        CREATE(pAssembly->pComponents, COMPONENT, pAssembly->lNumComponents + 1);
    else
        RECREATE(pAssembly->pComponents, COMPONENT, pAssembly->lNumComponents + 1);

    /*
     * Assign the new component table and setup the new component entry. Then
     * add increment the number of components.
     */

    pAssembly->pComponents[pAssembly->lNumComponents].lVnum = lComponentVnum;
    pAssembly->pComponents[pAssembly->lNumComponents].bExtract = bExtract;
    pAssembly->pComponents[pAssembly->lNumComponents].bInRoom = bInRoom;
    pAssembly->lNumComponents += 1;

    return (true);
}

bool assemblyCheckComponents(long lVnum, struct char_data *pCharacter, int extract_yes) {
    bool bOk = true;
    long i = 0;
    long lRnum = 0;
    struct obj_data **ppComponentObjects = nullptr;
    ASSEMBLY *pAssembly = nullptr;

    if (pCharacter == nullptr) {
        basic_mud_log("SYSERR: nullptr assemblyCheckComponents(): 'pCharacter'.");
        return (false);
    } else if ((pAssembly = assemblyGetAssemblyPtr(lVnum)) == nullptr) {
        basic_mud_log("SYSERR: nullptr assemblyCheckComponents(): Invalid 'lVnum' #%ld.", lVnum);
        return (false);
    }

    if (pAssembly->pComponents == nullptr)
        return (false);
    else if (pAssembly->lNumComponents <= 0)
        return (false);

    CREATE(ppComponentObjects, struct obj_data*, pAssembly->lNumComponents);

    for (i = 0; i < pAssembly->lNumComponents && bOk; i++) {
        if ((lRnum = real_object(pAssembly->pComponents[i].lVnum)) < 0)
            bOk = false;
        else {
            if (pAssembly->pComponents[i].bInRoom) {
                if ((ppComponentObjects[i] = pCharacter->location.findObjectVnum(lRnum)) == nullptr)
                    bOk = false;
                else {
                    ppComponentObjects[i]->clearLocation();
                }
            } else {
                if ((ppComponentObjects[i] = pCharacter->findObjectVnum(lRnum)) == nullptr)
                    bOk = false;
                else {
                    ppComponentObjects[i]->clearLocation();
                }
            }
        }
    }

    for (i = 0; i < pAssembly->lNumComponents; i++) {
        if (ppComponentObjects[i] == nullptr)
            continue;

        if (pAssembly->pComponents[i].bExtract && bOk && extract_yes == true)
            extract_obj(ppComponentObjects[i]);
        else if (pAssembly->pComponents[i].bInRoom)
            ppComponentObjects[i]->setLocation(pCharacter);
        else
            obj_to_char(ppComponentObjects[i], pCharacter);
    }

    free(ppComponentObjects);

    return (bOk);
}

bool assemblyCreate(long lVnum, int iAssembledType) {
    long lBottom = 0;
    long lMiddle = 0;
    long lTop = 0;
    ASSEMBLY *pNewAssemblyTable = nullptr;

    if (lVnum < 0)
        return (false);
    else if (iAssembledType < 0 || iAssembledType >= MAX_ASSM)
        return (false);

    if (g_pAssemblyTable == nullptr) {
        CREATE(g_pAssemblyTable, ASSEMBLY, 1);
        g_lNumAssemblies = 1;
    } else {
        lTop = g_lNumAssemblies - 1;

        for (;;) {
            lMiddle = (lBottom + lTop) / 2;

            if (g_pAssemblyTable[lMiddle].lVnum == lVnum)
                return (false);
            else if (lBottom >= lTop)
                break;
            else if (g_pAssemblyTable[lMiddle].lVnum > lVnum)
                lTop = lMiddle - 1;
            else
                lBottom = lMiddle + 1;
        }

        if (g_pAssemblyTable[lMiddle].lVnum <= lVnum)
            lMiddle += 1;

        CREATE(pNewAssemblyTable, ASSEMBLY, g_lNumAssemblies + 1);

        if (lMiddle > 0)
            memmove(pNewAssemblyTable, g_pAssemblyTable, lMiddle * sizeof(ASSEMBLY
            ));

        if (lMiddle <= g_lNumAssemblies - 1)
            memmove(pNewAssemblyTable + lMiddle + 1, g_pAssemblyTable + lMiddle,
                    (g_lNumAssemblies - lMiddle) * sizeof(ASSEMBLY));

        free(g_pAssemblyTable);
        g_pAssemblyTable = pNewAssemblyTable;
        g_lNumAssemblies += 1;
    }

    g_pAssemblyTable[lMiddle].lNumComponents = 0;
    g_pAssemblyTable[lMiddle].lVnum = lVnum;
    g_pAssemblyTable[lMiddle].pComponents = nullptr;
    g_pAssemblyTable[lMiddle].uchAssemblyType = (unsigned char) iAssembledType;
    return (true);
}

bool assemblyDestroy(long lVnum) {
    long lIndex = 0;
    ASSEMBLY *pNewAssemblyTable = nullptr;

    /* Find the real number of the assembled vnum. */
    if (g_pAssemblyTable == nullptr || (lIndex = assemblyGetAssemblyIndex(lVnum))
                                       < 0) {
        basic_mud_log("SYSERR: assemblyDestroy(): Invalid 'lVnum' #%ld.", lVnum);
        return (false);
    }

    /* Deallocate component array. */
    if (g_pAssemblyTable[lIndex].pComponents)
        free(g_pAssemblyTable[lIndex].pComponents);

    if (g_lNumAssemblies > 1) {
        /* Create a new table, the same size as the old one less one item. */
        CREATE(pNewAssemblyTable, ASSEMBLY, g_lNumAssemblies - 1);

        /* Copy all assemblies before the one removed into the new table. */
        if (lIndex > 0)
            memmove(pNewAssemblyTable, g_pAssemblyTable, lIndex * sizeof(ASSEMBLY));

        /* Copy all assemblies after the one removed into the new table. */
        if (lIndex < g_lNumAssemblies - 1) {
            memmove(pNewAssemblyTable + lIndex, g_pAssemblyTable + lIndex + 1, (g_lNumAssemblies - lIndex - 1) *
                                                                               sizeof(ASSEMBLY));
        }
    }

    /* Deallocate the old table. */
    free(g_pAssemblyTable);

    /* Decrement the assembly count and assign the new table. */
    g_lNumAssemblies -= 1;
    g_pAssemblyTable = pNewAssemblyTable;

    return (true);
}

bool assemblyHasComponent(long lVnum, long lComponentVnum) {
    ASSEMBLY *pAssembly = nullptr;

    if ((pAssembly = assemblyGetAssemblyPtr(lVnum)) == nullptr) {
        basic_mud_log("SYSERR: assemblyHasComponent(): Invalid 'lVnum' #%ld.", lVnum);
        return (false);
    }

    return (assemblyGetComponentIndex(pAssembly, lComponentVnum) >= 0);
}

bool assemblyRemoveComponent(long lVnum, long lComponentVnum) {
    long lIndex = 0;
    ASSEMBLY *pAssembly = nullptr;
    COMPONENT *pNewComponents = nullptr;

    if ((pAssembly = assemblyGetAssemblyPtr(lVnum)) == nullptr) {
        basic_mud_log("SYSERR: assemblyRemoveComponent(): Invalid 'lVnum' #%ld.", lVnum);
        return (false);
    } else if ((lIndex = assemblyGetComponentIndex(pAssembly, lComponentVnum)) <
               0) {
        basic_mud_log("SYSERR: assemblyRemoveComponent(): Vnum #%ld is not a "
            "component of assembled vnum #%ld.", lComponentVnum, lVnum);
        return (false);
    }

    if (pAssembly->pComponents && pAssembly->lNumComponents > 1) {
        CREATE(pNewComponents, COMPONENT, pAssembly->lNumComponents - 1);

        if (lIndex > 0)
            memmove(pNewComponents, pAssembly->pComponents, lIndex * sizeof(COMPONENT));

        if (lIndex < pAssembly->lNumComponents - 1) {
            memmove(pNewComponents + lIndex, pAssembly->pComponents + lIndex + 1,
                    (pAssembly->lNumComponents - lIndex - 1) * sizeof(COMPONENT));
        }
    }

    free(pAssembly->pComponents);
    pAssembly->pComponents = pNewComponents;
    pAssembly->lNumComponents -= 1;

    return (true);
}

int assemblyGetType(long lVnum) {
    ASSEMBLY *pAssembly = nullptr;

    if ((pAssembly = assemblyGetAssemblyPtr(lVnum)) == nullptr) {
        basic_mud_log("SYSERR: assemblyGetType(): Invalid 'lVnum' #%ld.", lVnum);
        return (-1);
    }

    return ((int) pAssembly->uchAssemblyType);
}

long assemblyCountComponents(long lVnum) {
    ASSEMBLY *pAssembly = nullptr;

    if ((pAssembly = assemblyGetAssemblyPtr(lVnum)) == nullptr) {
        basic_mud_log("SYSERR: assemblyCountComponents(): Invalid 'lVnum' #%ld.", lVnum);
        return (0);
    }

    return (pAssembly->lNumComponents);
}

long assemblyFindAssembly(const char *pszAssemblyName) {
    long i = 0;
    long lRnum = NOTHING;

    if (g_pAssemblyTable == nullptr)
        return (-1);
    else if (pszAssemblyName == nullptr || *pszAssemblyName == '\0')
        return (-1);

    for (i = 0; i < g_lNumAssemblies; i++) {
        if ((lRnum = real_object(g_pAssemblyTable[i].lVnum)) < 0)
            basic_mud_log("SYSERR: assemblyFindAssembly(): Invalid vnum #%ld in assembly table.", g_pAssemblyTable[i].lVnum);
        else if (isname(pszAssemblyName, obj_proto.at(lRnum).name))
            return (g_pAssemblyTable[i].lVnum);
    }

    return (-1);
}

long assemblyGetAssemblyIndex(long lVnum) {
    long lBottom = 0;
    long lMiddle = 0;
    long lTop = 0;

    lTop = g_lNumAssemblies - 1;

    for (;;) {
        lMiddle = (lBottom + lTop) / 2;

        if (g_pAssemblyTable[lMiddle].lVnum == lVnum)
            return (lMiddle);
        else if (lBottom >= lTop)
            return (-1);
        else if (g_pAssemblyTable[lMiddle].lVnum > lVnum)
            lTop = lMiddle - 1;
        else
            lBottom = lMiddle + 1;
    }
}

long assemblyGetComponentIndex(ASSEMBLY *pAssembly, long lComponentVnum) {
    long i = 0;

    if (pAssembly == nullptr)
        return (-1);

    for (i = 0; i < pAssembly->lNumComponents; i++) {
        if (pAssembly->pComponents[i].lVnum == lComponentVnum)
            return (i);
    }

    return (-1);
}

ASSEMBLY *assemblyGetAssemblyPtr(long lVnum) {
    long lIndex = 0;

    if (g_pAssemblyTable == nullptr)
        return (nullptr);

    if ((lIndex = assemblyGetAssemblyIndex(lVnum)) >= 0)
        return (g_pAssemblyTable + lIndex);

    return (nullptr);
}

#undef __ASSEMBLIES_C__

void free_assemblies() {
    int i;

    if (g_pAssemblyTable == nullptr)
        return;

    for (i = 0; i < g_lNumAssemblies; i++) {
        if (g_pAssemblyTable[i].pComponents)
            free(g_pAssemblyTable[i].pComponents);

        g_pAssemblyTable[i].lNumComponents = 0;
    }

    free(g_pAssemblyTable);
}

