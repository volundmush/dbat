#pragma once

/* Assembly type: Used in ASSEMBLY.iAssemblyType */
constexpr int ASSM_MAKE              = 0; // Assembly must be made.
constexpr int ASSM_BAKE              = 1; // Assembly must be baked.
constexpr int ASSM_BREW              = 2; // Assembly must be brewed.
constexpr int ASSM_ASSEMBLE          = 3; // Assembly must be assembled.
constexpr int ASSM_CRAFT             = 4; // Assembly must be crafted.
constexpr int ASSM_FLETCH            = 5; // Assembly must be fletched.
constexpr int ASSM_KNIT              = 6; // Assembly must be knitted.
constexpr int ASSM_MIX               = 7; // Assembly must be mixed.
constexpr int ASSM_THATCH            = 8; // Assembly must be thatched.
constexpr int ASSM_WEAVE             = 9; // Assembly must be woven.
constexpr int ASSM_FORGE             = 10; // Assembly must be forged.

constexpr int MAX_ASSM    	        = 11;

extern const char *AssemblyTypes[MAX_ASSM+1];