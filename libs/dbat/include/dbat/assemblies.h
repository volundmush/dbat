#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/assemblytypes.h"

typedef struct assembly_data   ASSEMBLY;
typedef struct component_data  COMPONENT;

/* ******************************************************************** *
 * Structure definitions.                                              *
 * ******************************************************************** */

/* Assembly structure definition. */
struct assembly_data {
  long         lVnum;                  /* Vnum of the object assembled. */
  long         lNumComponents;         /* Number of components. */
  unsigned char        uchAssemblyType;        /* Type of assembly (ASSM_xxx).
*/
  struct component_data *pComponents;          /* Array of component info. */
};

/* Assembly component structure definition. */
struct component_data {
  bool         bExtract;               /* Extract the object after use. */
  bool         bInRoom;                /* Component in room, not inven. */
  long         lVnum;                  /* Vnum of the component object. */
};

struct assembly_data alloc_assembly();


void           assemblyBootAssemblies( void );
void           assemblySaveAssemblies( void );
void           assemblyListToChar( struct char_data *pCharacter );

bool           assemblyAddComponent( long lVnum, long lComponentVnum,
                 bool bExtract, bool bInRoom );
bool           assemblyCheckComponents( long lVnum, struct char_data
                 *pCharacter, int extract_yes );
bool           assemblyCreate( long lVnum, int iAssembledType );
bool           assemblyDestroy( long lVnum );

int            assemblyGetType( long lVnum );

long           assemblyFindAssembly( const char *pszAssemblyName );
long           assemblyGetAssemblyIndex( long lVnum );

ASSEMBLY*      assemblyGetAssemblyPtr( long lVnum );

/* ******************************************************************** */

