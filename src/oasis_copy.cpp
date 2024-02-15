/******************************************************************************/
/** OasisOLC - InGame OLC Copying                                      v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include "dbat/oasis_copy.h"
#include "dbat/dg_comm.h"
#include "dbat/dg_olc.h"
#include "dbat/act.wizard.h"
#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/config.h"
#include "dbat/genolc.h"
#include "dbat/genwld.h"
#include "dbat/genshp.h"
#include "dbat/constants.h"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/

room_vnum redit_find_new_vnum(zone_rnum zone);


/* External Functions */



void redit_save_internally(struct descriptor_data *d);

void oedit_save_internally(struct descriptor_data *d);

void medit_save_internally(struct descriptor_data *d);

void sedit_save_internally(struct descriptor_data *d);

void trigedit_setup_existing(struct descriptor_data *d, int rnum);

void redit_setup_existing(struct descriptor_data *d, int rnum);

void oedit_setup_existing(struct descriptor_data *d, int rnum);

void medit_setup_existing(struct descriptor_data *d, int rnum);

void sedit_setup_existing(struct descriptor_data *d, vnum rshop_num);


/***********************************************************
* This function makes use of the high level OLC functions  *
* to copy most types of mud objects. The type of data is   *
* determined by the subcmd variable and the functions are  *
* looked up in a table.                                    *
***********************************************************/
ACMD(do_oasis_copy) {
    int i, src_vnum, src_rnum, dst_vnum, dst_rnum;
    char buf1[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];
    struct descriptor_data *d;

    struct {
        int con_type;

        IDXTYPE (*binary_search)(IDXTYPE vnum);

        void (*save_func)(struct descriptor_data *d);

        void (*setup_existing)(struct descriptor_data *d, int rnum);

        const char *command;
        const char *text;
    } oasis_copy_info[] = {
            {CON_REDIT,    real_room,    redit_save_internally, redit_setup_existing,    "rcopy", "room"},
            {CON_OEDIT,    real_object,  oedit_save_internally, oedit_setup_existing,    "ocopy", "object"},
            {CON_MEDIT,    real_mobile,  medit_save_internally, medit_setup_existing,    "mcopy", "mobile"},
            {CON_SEDIT,    real_shop,    sedit_save_internally, sedit_setup_existing,    "scopy", "shop"},
            {CON_TRIGEDIT, real_trigger, trigedit_save,         trigedit_setup_existing, "tcopy", "trigger"},
            {-1,           nullptr,      nullptr,               nullptr,                 "\n",    "\n"}
    };

    /* Find the given connection type in the table (passed in subcmd). */
    for (i = 0; *(oasis_copy_info[i].text) != '\n'; i++)
        if (subcmd == oasis_copy_info[i].con_type)
            break;
    /* If not found, we don't support copying that type of data. */
    if (*(oasis_copy_info[i].text) == '\n')
        return;

    /* No copying as a mob or while being forced. */
    if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
        return;

    /* We need two arguments. */
    two_arguments(argument, buf1, buf2);

    /* Both arguments are required and they must be numeric. */
    if (!*buf1 || !*buf2 || (!is_number(buf1) || !is_number(buf2))) {
        ch->sendf("Syntax: %s <source vnum> <target vnum>\r\n", oasis_copy_info[i].command);
        return;
    }

    /* We can't copy non-existing data. */
    /* Note: the source data can be in any zone. It's not restricted */
    /* to the builder's designated OLC zone. */
    src_vnum = atoi(buf1);
    src_rnum = (*oasis_copy_info[i].binary_search)(src_vnum);
    if (src_rnum == NOWHERE) {
        ch->sendf("The source %s (#%d) does not exist.\r\n", oasis_copy_info[i].text, src_vnum);
        return;
    }

    /* Don't copy if the target already exists. */
    dst_vnum = atoi(buf2);
    dst_rnum = (*oasis_copy_info[i].binary_search)(dst_vnum);
    if (dst_rnum != NOWHERE) {
        ch->sendf("The target %s (#%d) already exists.\r\n", oasis_copy_info[i].text, dst_vnum);
        return;
    }

    /* Check that whatever it is isn't already being edited. */
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) == subcmd) {
            if (d->olc && OLC_NUM(d) == dst_vnum) {
                ch->sendf("The target %s (#%d) is currently being edited by %s.\r\n",
                             oasis_copy_info[i].text, dst_vnum, GET_NAME(d->character));
                return;
            }
        }
    }

    d = ch->desc;

    /* Give the descriptor an OLC structure. */
    if (d->olc) {
        mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis_copy: Player already had olc structure.");
        free(d->olc);
    }

    /* Create the OLC structure. */
    CREATE(d->olc, struct oasis_olc_data, 1);

    /* Find the zone. */
    if ((OLC_ZNUM(d) = real_zone_by_thing(dst_vnum)) == NOWHERE) {
        ch->sendf("Sorry, there is no zone for the given vnum (#%d)!\r\n", dst_vnum);
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    /* Make sure the builder is allowed to modify the target zone. */
    if (!can_edit_zone(ch, OLC_ZNUM(d))) {
        send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    /* We tell the OLC functions that we want to save to the target vnum. */
    OLC_NUM(d) = dst_vnum;

    /* Perform the copy. */
    ch->sendf("Copying %s: source: #%d, dest: #%d.\r\n", oasis_copy_info[i].text, src_vnum, dst_vnum);
    (*oasis_copy_info[i].setup_existing)(d, src_rnum);
    (*oasis_copy_info[i].save_func)(d);

    /* Currently CLEANUP_ALL should be used for everything. */
    cleanup_olc(d, CLEANUP_ALL);
    ch->sendf("Done.\r\n");
}

/******************************************************************************/
/** Commands                                                                 **/
/******************************************************************************/

ACMD(do_dig) {
    char sdir[MAX_INPUT_LENGTH], sroom[MAX_INPUT_LENGTH], *new_room_name;
    room_vnum rvnum = NOWHERE;
    room_rnum rrnum = NOWHERE;
    zone_rnum zone;
    int dir = 0, rawvnum;
    struct descriptor_data *d = ch->desc; /* will save us some typing */

    /* Grab the room's name (if available). */
    new_room_name = two_arguments(argument, sdir, sroom);
    skip_spaces(&new_room_name);

    /* Can't dig if we don't know where to go. */
    if (!*sdir || !*sroom) {
        ch->sendf("Format: tunnel <direction> <room> - to create an exit\r\n"
                         "        tunnel <direction> -1     - to delete an exit\r\n");
        return;
    }

    rawvnum = atoi(sroom);
    if (rawvnum == -1)
        rvnum = NOWHERE;
    else
        rvnum = (room_vnum) rawvnum;
    rrnum = real_room(rvnum);
    if ((dir = search_block(sdir, abbr_dirs, false)) < 0)
        dir = search_block(sdir, dirs, false);
    
    auto r = ch->getRoom();
    zone = r->zone;

    if (dir < 0) {
        ch->sendf("Cannot create an exit to the '%s'.\r\n", sdir);
        return;
    }
    /* Make sure that the builder has access to the zone he's in. */
    if ((zone == NOWHERE) || !can_edit_zone(ch, zone)) {
        send_cannot_edit(ch, zone);
        return;
    }
    /*
     * Lets not allow digging to limbo.
     * After all, it'd just get us more errors on 'show errors'
     */
    if (rvnum == 0) {
        ch->sendf("The target exists, but you can't dig to limbo!\r\n");
        return;
    }
    
    auto e = W_EXIT(IN_ROOM(ch), dir);
    
    /*
     * target room == -1 removes the exit
     */
    if (rvnum == NOTHING) {
        if (e) {
            /* free the old pointers, if any */
            if (e->general_description)
                free(e->general_description);
            if (e->keyword)
                free(e->keyword);
            free(e);
            e = nullptr;
            r->save();
            ch->sendf("You remove the exit to the %s.\r\n", dirs[dir]);
            return;
        }
        ch->sendf("There is no exit to the %s.\r\n"
                         "No exit removed.\r\n", dirs[dir]);
        return;
    }
    /*
     * Can't dig in a direction, if it's already a door.
     */
    if (e) {
        ch->sendf("There already is an exit to the %s.\r\n", dirs[dir]);
        return;
    }

    /* Make sure that the builder has access to the zone he's linking to. */
    zone = real_zone_by_thing(rvnum);
    if (zone == NOWHERE) {
        ch->sendf("You cannot link to a non-existing zone!\r\n");
        return;
    }
    if (!can_edit_zone(ch, zone)) {
        send_cannot_edit(ch, zone);
        return;
    }
    /*
     * Now we know the builder is allowed to make the link
     */
    /* If the room doesn't exist, create it.*/
    if (rrnum == NOWHERE) {
        /*
         * Give the descriptor an olc struct.
         * This way we can let redit_save_internally handle the room adding.
         */
        if (d->olc) {
            mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: do_dig: Player already had olc structure.");
            free(d->olc);
        }
        CREATE(d->olc, struct oasis_olc_data, 1);
        OLC_ZNUM(d) = zone;
        OLC_NUM(d) = rvnum;
        OLC_ROOM(d) = new room_data();


        /* Copy the room's name. */
        if (*new_room_name)
            OLC_ROOM(d)->name = strdup(new_room_name);
        else
            OLC_ROOM(d)->name = strdup("An unfinished room");

        /* Copy the room's description.*/
        OLC_ROOM(d)->look_description = strdup("You are in an unfinished room.\r\n");
        OLC_ROOM(d)->zone = OLC_ZNUM(d);
        OLC_ROOM(d)->vn = NOWHERE;

        /*
         * Save the new room to memory.
         * redit_save_internally handles adding the room in the right place, etc.
         */
        redit_save_internally(d);
        OLC_VAL(d) = 0;

        ch->sendf("New room (%d) created.\r\n", rvnum);
        cleanup_olc(d, CLEANUP_ALL);
        update_space();
        /*
         * update rrnum to the correct room rnum after adding the room
         */
        rrnum = real_room(rvnum);
    }

    /*
     * Now dig.
     */
    r->dir_option[dir] = new room_direction_data();
    e = r->dir_option[dir];
    e->general_description = nullptr;
    e->keyword = nullptr;
    e->to_room = rrnum;
    r->save();
    ch->sendf("You make an exit %s to room %d (%s).\r\n",
                 dirs[dir], rvnum, world[rrnum]->name);

    /*
     * check if we can dig from there to here.
     */
    auto r2 = dynamic_cast<room_data*>(world[rrnum]);
    auto e2 = r2->dir_option[rev_dir[dir]];

    if (e2)
        ch->sendf("You cannot dig from %d to here. The target room already has an exit to the %s.\r\n",
                     rvnum, dirs[rev_dir[dir]]);
    else {
        r2->dir_option[rev_dir[dir]] = new room_direction_data();
        e2 = r2->dir_option[rev_dir[dir]];
        e2->general_description = nullptr;
        e2->keyword = nullptr;
        e2->to_room = IN_ROOM(ch);
    }
}

ACMD(do_rcopy) {
    
}

/****************************************************************************
* BuildWalk - OasisOLC Extension by D. Tyler Barnes                         *
****************************************************************************/

/* For buildwalk. Finds the next free vnum in the zone */
room_vnum redit_find_new_vnum(zone_rnum zone) {
    auto &z = zone_table[zone];
    for(auto i = z.bot; i < z.top; i++) {
        if(!world.count(i)) return i;
    }
    return NOWHERE;
}

int buildwalk(struct char_data *ch, int dir) {
    char buf[MAX_INPUT_LENGTH];
    room_vnum vnum;
    room_rnum rnum;

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_BUILDWALK) &&
        GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {

        if (!can_edit_zone(ch, ch->getRoom()->zone)) {
            send_cannot_edit(ch, ch->getRoom()->zone);
        } else if ((vnum = redit_find_new_vnum(ch->getRoom()->zone)) == NOWHERE)
            ch->sendf("No free vnums are available in this zone!\r\n");
        else {
            struct descriptor_data *d = ch->desc;
            /*
             * Give the descriptor an olc struct.
             * This way we can let redit_save_internally handle the room adding.
             */
            if (d->olc) {
                mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: buildwalk(): Player already had olc structure.");
                free(d->olc);
            }
            CREATE(d->olc, struct oasis_olc_data, 1);
            OLC_ZNUM(d) = ch->getRoom()->zone;
            OLC_NUM(d) = vnum;
            CREATE(OLC_ROOM(d), struct room_data, 1);

            OLC_ROOM(d)->name = strdup("New BuildWalk Room");

            sprintf(buf, "This unfinished room was created by %s.\r\n", GET_NAME(ch));
            OLC_ROOM(d)->look_description = strdup(buf);
            OLC_ROOM(d)->zone = OLC_ZNUM(d);
            OLC_ROOM(d)->vn = NOWHERE;

            /*
             * Save the new room to memory.
             * redit_save_internally handles adding the room in the right place, etc.
             */
            redit_save_internally(d);
            OLC_VAL(d) = 0;

            /* Link rooms */
            rnum = real_room(vnum);
            ch->getRoom()->dir_option[dir] = new room_direction_data();
            EXIT(ch, dir)->to_room = rnum;
            auto r = dynamic_cast<room_data*>(world[rnum]);

            r->dir_option[rev_dir[dir]] = new room_direction_data();
            r->dir_option[rev_dir[dir]]->to_room = IN_ROOM(ch);

            /* Report room creation to user */
            ch->sendf("@yRoom #%d created by BuildWalk.@n\r\n", vnum);
            cleanup_olc(d, CLEANUP_STRUCTS);
            update_space();
            return (1);

        }
    }

    return (0);
}

