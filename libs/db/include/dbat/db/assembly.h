#pragma once

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

