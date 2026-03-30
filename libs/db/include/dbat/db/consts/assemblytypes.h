#pragma once

/* Assembly type: Used in ASSEMBLY.iAssemblyType */
#define ASSM_MAKE              0       // Assembly must be made.
#define ASSM_BAKE              1       // Assembly must be baked.
#define ASSM_BREW              2       // Assembly must be brewed.
#define ASSM_ASSEMBLE          3       // Assembly must be assembled.
#define ASSM_CRAFT             4       // Assembly must be crafted.
#define ASSM_FLETCH            5       // Assembly must be fletched.
#define ASSM_KNIT              6       // Assembly must be knitted.
#define ASSM_MIX               7       // Assembly must be mixed.
#define ASSM_THATCH            8       // Assembly must be thatched.
#define ASSM_WEAVE             9       // Assembly must be woven.
#define ASSM_FORGE             10      // Assembly must be forged.

#define MAX_ASSM    	        11

extern const char *AssemblyTypes[MAX_ASSM+1];