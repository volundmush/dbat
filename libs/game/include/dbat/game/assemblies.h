/* ******************************************************************** *
 * FILE        : assemblies.h                  Copyright (C) 1999 Geoff Davis
*
 * USAGE: Definitions, constants and prototypes for assembly engine.   *
 * -------------------------------------------------------------------- *
 * 1999 MAY 07 gdavis/azrael@laker.net Initial implementation.         *
 * ******************************************************************** */

#ifndef __ASSEMBLIES_H__
#define __ASSEMBLIES_H__

#include "structs.h"

/* ******************************************************************** *
 * Preprocessor constants.                                             *
 * ******************************************************************** */


/* ******************************************************************** *
 * Type aliases.                                                       *
 * ******************************************************************** */

/* ******************************************************************** *
 * Prototypes for assemblies.c.
*
 * ******************************************************************** */

void           assemblyBootAssemblies( void );
void           assemblySaveAssemblies( void );
void           assemblyListToChar( struct char_data *pCharacter );

bool           assemblyAddComponent( long lVnum, long lComponentVnum,
                 bool bExtract, bool bInRoom );
bool           assemblyCheckComponents( long lVnum, struct char_data
                 *pCharacter, int extract_yes );
bool           assemblyCreate( long lVnum, int iAssembledType );
bool           assemblyDestroy( long lVnum );
bool           assemblyHasComponent( long lVnum, long lComponentVnum );
bool           assemblyRemoveComponent( long lVnum, long lComponentVnum );

int            assemblyGetType( long lVnum );

long           assemblyCountComponents( long lVnum );
long           assemblyFindAssembly( const char *pszAssemblyName );
long           assemblyGetAssemblyIndex( long lVnum );
long           assemblyGetComponentIndex( ASSEMBLY *pAssembly,
                 long lComponentVnum );

ASSEMBLY*      assemblyGetAssemblyPtr( long lVnum );

/* ******************************************************************** */

#endif

