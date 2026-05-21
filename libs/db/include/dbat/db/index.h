#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct index_data {
   mob_vnum	vnum;	/* virtual number of this mob/obj		*/
   int		number;	/* number of existing units of this mob/obj	*/
   SPECIAL(*func);

   char *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};

typedef struct index_data index_data;

#ifdef __cplusplus
}
#endif
