/* ******************************************************************** *
 * FILE        : assemblies.c                  Copyright (C) 1999 Geoff Davis
*
 * USAGE: Implementation for assembly engine.                          *
 * -------------------------------------------------------------------- *
 * 1999 MAY 07 gdavis/azrael@laker.net Initial implementation.         *
 * ******************************************************************** */
#include "dbat/assemblies.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/assemblytypes.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/fileop.hpp"
#include "dbat/db.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/log.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/extract.hpp"
#include "dbat/relocate.hpp"
#include "dbat/search.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

static long assemblyGetAssemblyIndex(GameWorld *g, obj_vnum lVnum);

/* Local global variables. */

void assemblyBootAssemblies(GameWorld *g)
{
  char szLine[MAX_STRING_LENGTH] = {'\0'};
  char szTag[MAX_STRING_LENGTH]  = {'\0'};
  char szType[MAX_STRING_LENGTH] = {'\0'};
  int iExtract                   = 0;
  int iInRoom                    = 0;
  int iType                      = 0;
  long lLineCount                = 0;
  obj_vnum lPartVnum             = NOTHING;
  obj_vnum lVnum                 = NOTHING;
  FILE *pFile                    = nullptr;

  if ((pFile = fopen(ASSEMBLIES_FILE, "rt")) == nullptr) {
    mud_log("SYSERR: assemblyBootAssemblies(): Couldn't open file '%s' for "
            "reading.",
            ASSEMBLIES_FILE);
    return;
  }

  while (!feof(pFile)) {
    lLineCount += get_line(pFile, szLine);
    half_chop(szLine, szTag, szLine);

    if (*szTag == '\0')
      continue;

    if (strcasecmp(szTag, "Component") == 0) {
      if (sscanf(szLine, "#%d %d %d", &lPartVnum, &iExtract, &iInRoom) != 3) {
        mud_log("SYSERR: bootAssemblies(): Invalid format in file %s, line %ld: "
                "szTag=%s, szLine=%s.",
                ASSEMBLIES_FILE, lLineCount, szTag, szLine);
      } else if (!assemblyAddComponent(g, lVnum, lPartVnum, iExtract, iInRoom)) {
        mud_log("SYSERR: bootAssemblies(): Could not add component #%d to "
                "assembly #%d.",
                lPartVnum, lVnum);
      }
    } else if (strcasecmp(szTag, "Vnum") == 0) {
      if (sscanf(szLine, "#%d %s", &lVnum, szType) != 2) {
        mud_log("SYSERR: bootAssemblies(): Invalid format in file %s, "
                "line %ld.",
                ASSEMBLIES_FILE, lLineCount);
        lVnum = NOTHING;
      } else if ((iType = search_block(szType, AssemblyTypes, true)) < 0) {
        mud_log("SYSERR: bootAssemblies(): Invalid type '%s' for assembly "
                "vnum #%d at line %ld.",
                szType, lVnum, lLineCount);
        lVnum = NOTHING;
      } else if (!assemblyCreate(g, lVnum, iType)) {
        mud_log("SYSERR: bootAssemblies(): Could not create assembly for vnum "
                "#%d, type %s.",
                lVnum, szType);
        lVnum = NOTHING;
      }
    } else {
      mud_log("SYSERR: Invalid tag '%s' in file %s, line #%ld.", szTag,
              ASSEMBLIES_FILE, lLineCount);
    }

    *szLine = '\0';
    *szTag  = '\0';
  }

  fclose(pFile);
}

void assemblySaveAssemblies(GameWorld *g)
{
  struct atomic_file af;
  char szType[MAX_STRING_LENGTH] = {'\0'};
  long i                         = 0;
  long j                         = 0;
  ASSEMBLY *pAssembly            = nullptr;
  FILE *pFile                    = nullptr;

  if ((pFile = atomic_open(&af, ASSEMBLIES_FILE, false)) == nullptr) {
    mud_log("SYSERR: assemblySaveAssemblies(): Couldn't open file '%s' for "
            "writing.",
            ASSEMBLIES_FILE);
    return;
  }

  for (i = 0; i < g->g_lNumAssemblies; i++) {
    pAssembly = (g->g_pAssemblyTable + i);
    sprinttype(pAssembly->uchAssemblyType, AssemblyTypes, szType, sizeof(szType));
    fprintf(pFile, "Vnum                #%d %s\n", pAssembly->lVnum, szType);

    for (j = 0; j < pAssembly->lNumComponents; j++) {
      fprintf(pFile, "Component           #%d %d %d\n",
              pAssembly->pComponents[j].lVnum,
              (pAssembly->pComponents[j].bExtract ? 1 : 0),
              (pAssembly->pComponents[j].bInRoom ? 1 : 0));
    }

    if (i < g->g_lNumAssemblies - 1)
      fprintf(pFile, "\n");
  }

  atomic_commit(&af);
}

void assemblyListToChar(GameWorld *g, struct char_data *pCharacter)
{
  char szBuffer[MAX_STRING_LENGTH]  = {'\0'};
  char szAssmType[MAX_INPUT_LENGTH] = {'\0'};
  long i                            = 0; // Outer iterator.
  long j                            = 0; // Inner iterator.
  obj_rnum lRnum                    = 0; // Object rnum for obj_proto indexing.

  if (pCharacter == nullptr) {
    mud_log("SYSERR: assemblyListAssembliesToChar(): NULL 'pCharacter'.");
    return;
  }
  if (g->g_pAssemblyTable == nullptr) {
    send_to_char(g, pCharacter, "No assemblies exist.\r\n");
    return;
  }

  /* Send out a "header" of sorts. */
  send_to_char(g, pCharacter, "The following assemblies exists:\r\n");

  for (i = 0; i < g->g_lNumAssemblies; i++) {
    if ((lRnum = real_object(g, g->g_pAssemblyTable[i].lVnum)) < 0) {
      send_to_char(g, pCharacter, "[-----] ***RESERVED***\r\n");
      mud_log("SYSERR: assemblyListToChar(): Invalid vnum #%d in assembly table.", g->g_pAssemblyTable[i].lVnum);
    } else {
      sprinttype(g->g_pAssemblyTable[i].uchAssemblyType, AssemblyTypes, szAssmType, sizeof(szAssmType));
      sprintf(szBuffer, "[%5d] %s (%s)\r\n", g->g_pAssemblyTable[i].lVnum,
              obj_get_short_description(&g->obj_proto[lRnum]), szAssmType);
      send_to_char(g, pCharacter, "%s", szBuffer);

      for (j = 0; j < g->g_pAssemblyTable[i].lNumComponents; j++) {
        if ((lRnum = real_object(g, g->g_pAssemblyTable[i].pComponents[j].lVnum)) < 0) {
          send_to_char(g, pCharacter, " -----: ***RESERVED***\r\n");
          mud_log("SYSERR: assemblyListToChar(): Invalid component vnum #%d in assembly for vnum #%d.",
                  g->g_pAssemblyTable[i].pComponents[j].lVnum, g->g_pAssemblyTable[i].lVnum);
        } else {
          sprintf(szBuffer, " %5d: %-20.20s Extract=%-3.3s InRoom=%-3.3s\r\n", +g->g_pAssemblyTable[i].pComponents[j].lVnum,
                  obj_get_short_description(&g->obj_proto[lRnum]),
                  (g->g_pAssemblyTable[i].pComponents[j].bExtract ? "Yes" : "No"),
                  (g->g_pAssemblyTable[i].pComponents[j].bInRoom ? "Yes" : "No"));
          send_to_char(g, pCharacter, "%s", szBuffer);
        }
      }
    }
  }
}

bool assemblyAddComponent(GameWorld *g, obj_vnum lVnum, obj_vnum lComponentVnum, bool bExtract, bool bInRoom)
{
  ASSEMBLY *pAssembly = nullptr;

  if ((pAssembly = assemblyGetAssemblyPtr(g, lVnum)) == nullptr) {
    mud_log("SYSERR: assemblyAddComponent(): Invalid 'lVnum' #%d.", lVnum);
    return (false);
  }
  if (real_object(g, lComponentVnum) <= NOTHING) {
    mud_log("SYSERR: assemblyAddComponent(): Invalid 'lComponentVnum' #%d.",
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

  pAssembly->pComponents[pAssembly->lNumComponents].lVnum    = lComponentVnum;
  pAssembly->pComponents[pAssembly->lNumComponents].bExtract = bExtract;
  pAssembly->pComponents[pAssembly->lNumComponents].bInRoom  = bInRoom;
  pAssembly->lNumComponents += 1;

  return (true);
}

bool assemblyCheckComponents(GameWorld *g, obj_vnum lVnum, struct char_data *pCharacter, int extract_yes)
{
  bool bOk                             = true;
  long i                               = 0;
  obj_rnum lRnum                       = 0;
  struct obj_data **ppComponentObjects = nullptr;
  ASSEMBLY *pAssembly                  = nullptr;

  if (pCharacter == nullptr) {
    mud_log("SYSERR: NULL assemblyCheckComponents(): 'pCharacter'.");
    return (false);
  }
  if ((pAssembly = assemblyGetAssemblyPtr(g, lVnum)) == nullptr) {
    mud_log("SYSERR: NULL assemblyCheckComponents(): Invalid 'lVnum' #%d.", lVnum);
    return (false);
  }

  if (pAssembly->pComponents == nullptr)
    return (false);
  else if (pAssembly->lNumComponents <= 0)
    return (false);

  CREATE(ppComponentObjects, struct obj_data *, pAssembly->lNumComponents);

  for (i = 0; i < pAssembly->lNumComponents && bOk; i++) {
    if ((lRnum = real_object(g, pAssembly->pComponents[i].lVnum)) < 0)
      bOk = false;
    else {
      if (pAssembly->pComponents[i].bInRoom) {
        if ((ppComponentObjects[i] = get_obj_in_list_num(lRnum,
                                                         rroom_get_contents(g, char_get_in_room(pCharacter)))) == nullptr)
          bOk = false;
        else {
          obj_from_room(g, ppComponentObjects[i]);
        }
      } else {
        if ((ppComponentObjects[i] = get_obj_in_list_num(lRnum,
                                                         char_get_inventory(pCharacter))) == nullptr)
          bOk = false;
        else {
          obj_from_char(g, ppComponentObjects[i]);
        }
      }
    }
  }

  for (i = 0; i < pAssembly->lNumComponents; i++) {
    if (ppComponentObjects[i] == nullptr)
      continue;

    if (pAssembly->pComponents[i].bExtract && bOk && extract_yes == true)
      extract_obj(g, ppComponentObjects[i]);
    else if (pAssembly->pComponents[i].bInRoom)
      obj_to_room(g, ppComponentObjects[i], char_get_in_room(pCharacter));
    else
      obj_to_char(g, ppComponentObjects[i], pCharacter);
  }

  free(ppComponentObjects);

  return (bOk);
}

bool assemblyCreate(GameWorld *g, obj_vnum lVnum, int iAssembledType)
{
  long lBottom                = 0;
  long lMiddle                = 0;
  long lTop                   = 0;
  ASSEMBLY *pNewAssemblyTable = nullptr;

  if (lVnum < 0)
    return (false);
  else if (iAssembledType < 0 || iAssembledType >= MAX_ASSM)
    return (false);

  if (g->g_pAssemblyTable == nullptr) {
    CREATE(g->g_pAssemblyTable, ASSEMBLY, 1);
    g->g_lNumAssemblies = 1;
  } else {
    lTop = g->g_lNumAssemblies - 1;

    for (;;) {
      lMiddle = (lBottom + lTop) / 2;

      if (g->g_pAssemblyTable[lMiddle].lVnum == lVnum)
        return (false);
      else if (lBottom >= lTop)
        break;
      else if (g->g_pAssemblyTable[lMiddle].lVnum > lVnum)
        lTop = lMiddle - 1;
      else
        lBottom = lMiddle + 1;
    }

    if (g->g_pAssemblyTable[lMiddle].lVnum <= lVnum)
      lMiddle += 1;

    CREATE(pNewAssemblyTable, ASSEMBLY, g->g_lNumAssemblies + 1);

    if (lMiddle > 0)
      memmove(pNewAssemblyTable, g->g_pAssemblyTable, lMiddle * sizeof(ASSEMBLY));

    if (lMiddle <= g->g_lNumAssemblies - 1)
      memmove(pNewAssemblyTable + lMiddle + 1, g->g_pAssemblyTable + lMiddle, (g->g_lNumAssemblies - lMiddle) * sizeof(ASSEMBLY));

    free(g->g_pAssemblyTable);
    g->g_pAssemblyTable = pNewAssemblyTable;
    g->g_lNumAssemblies += 1;
  }

  g->g_pAssemblyTable[lMiddle].lNumComponents  = 0;
  g->g_pAssemblyTable[lMiddle].lVnum           = lVnum;
  g->g_pAssemblyTable[lMiddle].pComponents     = nullptr;
  g->g_pAssemblyTable[lMiddle].uchAssemblyType = (unsigned char)iAssembledType;
  return (true);
}

bool assemblyDestroy(GameWorld *g, obj_vnum lVnum)
{
  long lIndex                 = 0;
  ASSEMBLY *pNewAssemblyTable = nullptr;

  /* Find the real number of the assembled vnum. */
  if (g->g_pAssemblyTable == nullptr || (lIndex = assemblyGetAssemblyIndex(g, lVnum)) < 0) {
    mud_log("SYSERR: assemblyDestroy(): Invalid 'lVnum' #%d.", lVnum);
    return (false);
  }

  /* Deallocate component array. */
  if (g->g_pAssemblyTable[lIndex].pComponents != nullptr)
    free(g->g_pAssemblyTable[lIndex].pComponents);

  if (g->g_lNumAssemblies > 1) {
    /* Create a new table, the same size as the old one less one item. */
    CREATE(pNewAssemblyTable, ASSEMBLY, g->g_lNumAssemblies - 1);

    /* Copy all assemblies before the one removed into the new table. */
    if (lIndex > 0)
      memmove(pNewAssemblyTable, g->g_pAssemblyTable, lIndex * sizeof(ASSEMBLY));

    /* Copy all assemblies after the one removed into the new table. */
    if (lIndex < g->g_lNumAssemblies - 1) {
      memmove(pNewAssemblyTable + lIndex, g->g_pAssemblyTable + lIndex + 1, (g->g_lNumAssemblies - lIndex - 1) * sizeof(ASSEMBLY));
    }
  }

  /* Deallocate the old table. */
  free(g->g_pAssemblyTable);

  /* Decrement the assembly count and assign the new table. */
  g->g_lNumAssemblies -= 1;
  g->g_pAssemblyTable = pNewAssemblyTable;

  return (true);
}

int assemblyGetType(GameWorld *g, obj_vnum lVnum)
{
  ASSEMBLY *pAssembly = nullptr;

  if ((pAssembly = assemblyGetAssemblyPtr(g, lVnum)) == nullptr) {
    mud_log("SYSERR: assemblyGetType(): Invalid 'lVnum' #%d.", lVnum);
    return (-1);
  }

  return ((int)pAssembly->uchAssemblyType);
}

obj_vnum assemblyFindAssembly(GameWorld *g, const char *pszAssemblyName)
{
  long i         = 0;
  obj_rnum lRnum = NOTHING;

  if (g->g_pAssemblyTable == nullptr)
    return (-1);
  else if (pszAssemblyName == nullptr || *pszAssemblyName == '\0')
    return (-1);

  for (i = 0; i < g->g_lNumAssemblies; i++) {
    if ((lRnum = real_object(g, g->g_pAssemblyTable[i].lVnum)) < 0)
      mud_log("SYSERR: assemblyFindAssembly(): Invalid vnum #%d in assembly table.", g->g_pAssemblyTable[i].lVnum);
    else if (isname(pszAssemblyName, obj_get_name(&g->obj_proto[lRnum])))
      return (g->g_pAssemblyTable[i].lVnum);
  }

  return (-1);
}

static long assemblyGetAssemblyIndex(GameWorld *g, obj_vnum lVnum)
{
  long lBottom = 0;
  long lMiddle = 0;
  long lTop    = 0;

  lTop = g->g_lNumAssemblies - 1;

  for (;;) {
    lMiddle = (lBottom + lTop) / 2;

    if (g->g_pAssemblyTable[lMiddle].lVnum == lVnum)
      return (lMiddle);
    else if (lBottom >= lTop)
      return (-1);
    else if (g->g_pAssemblyTable[lMiddle].lVnum > lVnum)
      lTop = lMiddle - 1;
    else
      lBottom = lMiddle + 1;
  }
}

ASSEMBLY *assemblyGetAssemblyPtr(GameWorld *g, obj_vnum lVnum)
{
  long lIndex = 0;

  if (g->g_pAssemblyTable == nullptr)
    return (nullptr);

  if ((lIndex = assemblyGetAssemblyIndex(g, lVnum)) >= 0)
    return (g->g_pAssemblyTable + lIndex);

  return (nullptr);
}

#undef __ASSEMBLIES_C__

void free_assemblies(GameWorld *g)
{
  int i;

  if (g->g_pAssemblyTable == nullptr)
    return;

  for (i = 0; i < g->g_lNumAssemblies; i++) {
    if (g->g_pAssemblyTable[i].pComponents)
      free(g->g_pAssemblyTable[i].pComponents);

    g->g_pAssemblyTable[i].lNumComponents = 0;
  }

  free(g->g_pAssemblyTable);
}
