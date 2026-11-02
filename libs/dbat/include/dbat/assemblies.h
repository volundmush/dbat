#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/assemblytypes.h"

typedef struct assembly_data ASSEMBLY;
typedef struct component_data COMPONENT;

/* ******************************************************************** *
 * Structure definitions.                                              *
 * ******************************************************************** */

/* Assembly structure definition. */
struct assembly_data {
  obj_vnum lVnum;                     /* Vnum of the object assembled. */
  long lNumComponents;                /* Number of components. */
  unsigned char uchAssemblyType;      /* Type of assembly (ASSM_xxx).
*/
  struct component_data *pComponents; /* Array of component info. */
};

/* Assembly component structure definition. */
struct component_data {
  bool bExtract;  /* Extract the object after use. */
  bool bInRoom;   /* Component in room, not inven. */
  obj_vnum lVnum; /* Vnum of the component object. */
};

void assemblyBootAssemblies(void);
void assemblySaveAssemblies(void);
void assemblyListToChar(struct char_data *pCharacter);

bool assemblyAddComponent(obj_vnum lVnum, obj_vnum lComponentVnum,
                          bool bExtract, bool bInRoom);
bool assemblyCheckComponents(obj_vnum lVnum, struct char_data *pCharacter, int extract_yes);
bool assemblyCreate(obj_vnum lVnum, int iAssembledType);
bool assemblyDestroy(obj_vnum lVnum);

int assemblyGetType(obj_vnum lVnum);

obj_vnum assemblyFindAssembly(const char *pszAssemblyName);

ASSEMBLY *assemblyGetAssemblyPtr(obj_vnum lVnum);

/* ******************************************************************** */
void free_assemblies(void);
