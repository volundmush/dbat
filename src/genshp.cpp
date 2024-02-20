/************************************************************************
 * Generic OLC Library - Shops / genshp.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/genshp.h"
#include "dbat/utils.h"
#include "dbat/db.h"

#include "dbat/genolc.h"
#include "dbat/genshp.h"
#include "dbat/genzon.h"

/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_shop(struct Shop *tshop, struct Shop *fshop, int free_old_strings) {

}

/*-------------------------------------------------------------------*/

/*
 * Copy a 'NOTHING' terminated integer array list.
 */
void copy_list(IDXTYPE **tlist, IDXTYPE *flist) {

}

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/

void add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl) {

}

/*-------------------------------------------------------------------*/

void add_to_int_list(IDXTYPE **list, IDXTYPE newi) {

}

/*-------------------------------------------------------------------*/

void remove_from_int_list(IDXTYPE **list, IDXTYPE num) {
    IDXTYPE i, num_items, *nlist;

    /*
     * Count number of entries.
     */
    for (i = 0; (*list)[i] != NOTHING; i++);

#if CIRCLE_UNSIGNED_INDEX
    if (num >= i)
#else
        if (num < 0 || num >= i)
#endif
        return;
    num_items = i;

    CREATE(nlist, IDXTYPE, num_items);

    for (i = 0; i < num_items; i++)
        nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

    free(*list);
    *list = nlist;
}

/*-------------------------------------------------------------------*/

/*
 * Free all the notice character strings in a shop structure.
 */
void free_shop_strings(struct Shop *shop) {

}

/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/

/*
 * Free up the whole shop structure and it's content.
 */
void free_shop(struct Shop *shop) {

}

/*-------------------------------------------------------------------*/

/* returns the real number of the shop with given virtual number 
 *
 * We take so good care to keep it sorted - let's use it :) - Welcor
 */
shop_rnum real_shop(shop_vnum vnum) {
    return shop_index.count(vnum) ? vnum : NOTHING;
}

/*-------------------------------------------------------------------*/

/*
 * Generic string modifier for shop keeper messages.
 */
void modify_string(char **str, char *new_s) {

}

/*-------------------------------------------------------------------*/

int add_shop(struct Shop *nshp) {

}

/*-------------------------------------------------------------------*/

int save_shops(zone_rnum zone_num) {
    return true;
}
