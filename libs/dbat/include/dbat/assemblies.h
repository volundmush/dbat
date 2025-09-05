#pragma once

struct Character;

typedef struct assembly_data ASSEMBLY;
typedef struct component_data COMPONENT;



struct assembly_data {
    long lVnum;                  /* Vnum of the object assembled. */
    long lNumComponents;         /* Number of components. */
    unsigned char uchAssemblyType;        /* Type of assembly (ASSM_xxx).
*/
    struct component_data *pComponents;          /* Array of component info. */
};

/* Assembly component structure definition. */
struct component_data {
    bool bExtract;               /* Extract the object after use. */
    bool bInRoom;                /* Component in room, not inven. */
    long lVnum;                  /* Vnum of the component object. */
};

extern long g_lNumAssemblies;
extern ASSEMBLY *g_pAssemblyTable;

/* ******************************************************************** *
 * Preprocessor constants.                                             *
 * ******************************************************************** */

/* Assembly type: Used in ASSEMBLY.iAssemblyType */
constexpr int ASSM_MAKE = 0;     // Assembly must be made.
constexpr int ASSM_BAKE = 1;     // Assembly must be baked.
constexpr int ASSM_BREW = 2;     // Assembly must be brewed.
constexpr int ASSM_ASSEMBLE = 3; // Assembly must be assembled.
constexpr int ASSM_CRAFT = 4;    // Assembly must be crafted.
constexpr int ASSM_FLETCH = 5;   // Assembly must be fletched.
constexpr int ASSM_KNIT = 6;     // Assembly must be knitted.
constexpr int ASSM_MIX = 7;      // Assembly must be mixed.
constexpr int ASSM_THATCH = 8;   // Assembly must be thatched.
constexpr int ASSM_WEAVE = 9;    // Assembly must be woven.
constexpr int ASSM_FORGE = 10;   // Assembly must be forged.

/* ******************************************************************** *
 * Prototypes for assemblies.c.
 *
 * ******************************************************************** */

extern void assemblyListToChar(Character *pCharacter, int type = 0);

extern bool assemblyAddComponent(long lVnum, long lComponentVnum,
                                 bool bExtract, bool bInRoom);

extern bool assemblyCheckComponents(long lVnum, Character *pCharacter, int extract_yes);

extern bool assemblyCreate(long lVnum, int iAssembledType);

extern bool assemblyDestroy(long lVnum);

extern bool assemblyHasComponent(long lVnum, long lComponentVnum);

extern bool assemblyRemoveComponent(long lVnum, long lComponentVnum);

extern int assemblyGetType(long lVnum);

extern long assemblyCountComponents(long lVnum);

extern long assemblyFindAssembly(const char *pszAssemblyName);

extern long assemblyGetAssemblyIndex(long lVnum);

extern long assemblyGetComponentIndex(ASSEMBLY *pAssembly,
                                      long lComponentVnum);

ASSEMBLY *assemblyGetAssemblyPtr(long lVnum);

/* ******************************************************************** */
