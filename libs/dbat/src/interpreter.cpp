/* ************************************************************************
 *   File: interpreter.c                                 Part of CircleMUD *
 *  Usage: parse user commands, search for specials, call ACMD functions   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "dbat/CharacterUtils.h"
#include "dbat/ObjectUtils.h"
#include "dbat/Descriptor.h"
#include "dbat/RoomUtils.h"
#include "dbat/Account.h"
#include "dbat/Location.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/send.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/mail.h"
#include "dbat/dg_scripts.h"
#include "dbat/Guild.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/config.h"
#include "dbat/weather.h"
//#include "dbat/act.informative.h"
#include "dbat/players.h"
//#include "dbat/obj_edit.h"
#include "dbat/commands.h"
//#include "dbat/act.wizard.h"
#include "dbat/Command.h"
#include "dbat/utils.h"
#include "dbat/filter.h"
#include "dbat/Random.h"
#include "dbat/TimeInfo.h"

#include "dbat/const/AdminLevel.h"
#include "dbat/const/Position.h"
#include "dbat/const/Filename.h"

/* local functions */
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);

int reserved_word(char *argument);

int command_pass(char *cmd, Character *ch);

void payout(int num);

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

const char *fill[] =
    {
        "in",
        "into",
        "from",
        "with",
        "the",
        "on",
        "at",
        "to",
        "\n"};

const char *reserved[] =
    {
        "a",
        "an",
        "self",
        "me",
        "all",
        "room",
        "someone",
        "something",
        "\n"};

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(Character *ch, char *argument)
{
    int cmd, length;
    int skip_ld = 0;

    switch (GET_POS(ch))
    {
    case POS_DEAD:
    case POS_INCAP:
    case POS_MORTALLYW:
    case POS_STUNNED:
        ch->position = POS_SITTING;
        break;
    }

    /* just drop to next line for hitting CR */
    skip_spaces(&argument);
    if (!*argument)
        return;
    
    auto cdata = CommandData(argument);

    /* Since all command triggers check for valid_dg_target before acting, the levelcheck
     * here has been removed.
     */
    /* otherwise, find the command */
    {
        int cont;                               /* continue the command checks */
        cont = command_wtrigger(ch, cdata.cmd, cdata.argument); /* any world triggers ? */
        if (!cont)
            cont = command_mtrigger(ch, cdata.cmd, cdata.argument); /* any mobile triggers ? */
        if (!cont)
            cont = command_otrigger(ch, cdata.cmd, cdata.argument); /* any object triggers ? */
        if (cont)
            return; /* yes, command trigger took over */
    }

    cmd = matchCommand(ch, cdata.cmd);
    if(cmd < 0) {
        ch->sendText("Command not found.\r\n");
        return;
    }

    auto &cm = complete_cmd_info[cmd];

    if (!IS_NPC(ch) && cm.wait_list == 1)
    {
        if (ch->task != Task::nothing)
        {
            ch->sendText("Use '--' if you want to stop your current task.\r\n");
        }
        else
        {
            ch->wait_input_queue.emplace_back(cmd, cdata);
            characterSubscriptions.subscribe("commandWaitQueue", ch);
        }
        return;
    }

    processCommand(ch, cmd, cdata);
}

void commandWaitQueue(uint64_t heartPulse, double deltaTime)
{

    auto sub = characterSubscriptions.all("commandWaitQueue");

    for (auto ch : filter_raw(sub))
    {
        if (auto res = ch->modBaseStat("waitTime", -deltaTime); res <= 0.0)
        {
            if (ch->task != Task::nothing)
                doContinuedTask(ch);
            else if (!ch->wait_input_queue.empty())
            {
                auto [cmd, cdata] = ch->wait_input_queue.front();
                ch->wait_input_queue.pop_front();
                processCommand(ch, cmd, cdata);
            }
            if (ch->getBaseStat("waitTime") <= 0.0 && ch->task == Task::nothing && ch->wait_input_queue.empty())
            {
                characterSubscriptions.unsubscribe("commandWaitQueue", ch);
            }
        }
    }
}

void processCommand(Character *ch, int cmd, CommandData cd)
{
    char blah[MAX_INPUT_LENGTH];
    int skip_ld = 0;

    std::string line_str = std::string(cd.argument);
    auto line = (char*)line_str.c_str();

    auto cm = complete_cmd_info[cmd];

    sprintf(blah, "%s", cm.command);
    if (boost::iequals(blah, "throw"))
        ch->setBaseStat<int>("throws", Random::get<int>(1, 3));

    if (*cm.command == '\n')
    {
        ch->sendText("Huh!?!\r\n");
        return;
    }

    if (!cm.command_pointer)
    {
        ch->sendText("Sorry, that command hasn't been implemented yet.\r\n");
        return;
    }

    if (!command_pass(blah, ch) && GET_ADMLEVEL(ch) < 1)
    {
        ch->sendText("It's unfortunate...\r\n");
        return;
    }

    if (!IS_NPC(ch))
    {

        if (GET_ADMLEVEL(ch) < ADMLVL_IMPL)
        {
            if (PLR_FLAGGED(ch, PLR_GOOP))
            {
                ch->sendText("You only have your internal thoughts until your body has finished regenerating!\r\n");
                return;
            }

            if (PLR_FLAGGED(ch, PLR_FROZEN))
            {
                ch->sendText("You try, but the mind-numbing cold prevents you...\r\n");
                return;
            }
        }

        if (PLR_FLAGGED(ch, PLR_SPIRAL))
        {
            ch->sendText("You are occupied with your Spiral Comet attack!\r\n");
            return;
        }
    }
    else
    {
        if (cm.minimum_admlevel >= ADMLVL_IMMORT)
        {
            ch->sendText("You can't use immortal commands while switched.\r\n");
            return;
        }
    }

    if (auto minpos = cm.minimum_position; GET_POS(ch) < minpos && GET_POS(ch) != POS_FIGHTING)
    {
        switch (GET_POS(ch))
        {
        case POS_DEAD:
            ch->send_to("Lie still; you are DEAD!!! :-(\r\n");
            return;
        case POS_INCAP:
        case POS_MORTALLYW:
            ch->sendText("You are in a pretty bad shape, unable to do anything!\r\n");
            return;
        case POS_STUNNED:
            ch->sendText("All you can do right now is think about the stars!\r\n");
            return;
        case POS_SLEEPING:
            ch->sendText("In your dreams, or what?\r\n");
            return;
        case POS_RESTING:
            do_stand(ch, "stand", 0, 0);
            if (GET_POS(ch) != POS_STANDING)
            {
                ch->sendText("Nah... You feel too relaxed to do that..\r\n");
                return;
            }
            break;
        case POS_SITTING:
            do_stand(ch, "stand", 0, 0);
            if (GET_POS(ch) != POS_STANDING)
            {
                ch->sendText("Maybe you should get on your feet first?\r\n");
                return;
            }
        case POS_FIGHTING:
            ch->sendText("No way!  You're fighting for your life!\r\n");
            return;
        }
    }
    if (no_specials || !special(ch, cmd, line))
    {
        if (!skip_ld)
        {
            ((*cm.command_pointer)(ch, line, cmd, cm.subcmd, cd));
        }
    }
}

/**************************************************************************
 * Routines to handle aliasing                                             *
 **************************************************************************/

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
    char arg[MAX_INPUT_LENGTH];
    char *repl;
    struct alias_data *a, *temp;

    if (IS_NPC(ch))
        return;

    auto &p = players.at(ch->id);

    repl = any_one_arg(argument, arg);

    if (!*arg)
    { /* no argument specified -- list currently defined aliases */
        ch->sendText("Currently defined aliases:\r\n");
        int count = 0;
        for (auto &a : p->aliases)
        {
            count++;
            ch->send_to("%-15s %s\r\n", a.name.c_str(), a.replacement.c_str());
        }
        if (!count)
        {
            ch->sendText(" None.\r\n");
        }
        return;
    }
    /* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    auto &aliases = p->aliases;
    auto find = std::find_if(aliases.begin(), aliases.end(), [&](const auto &a)
                             { return boost::iequals(a.name, arg); });

    /* if no replacement string is specified, assume we want to delete */
    if (!*repl)
    {
        if (find == aliases.end())
            ch->sendText("No such alias.\r\n");
        else
        {
            aliases.erase(find);
            ch->sendText("Alias deleted.\r\n");
        }
        return;
    }

    /* otherwise, either add or redefine an alias */
    if (boost::iequals(arg, "alias"))
    {
        ch->sendText("You can't alias 'alias'.\r\n");
        return;
    }

    delete_doubledollar(repl);
    // type is ALIAS_SIMPLE if repl contains no ; otherwiise it's ALIAS_COMPLEX
    auto type = (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR)) ? ALIAS_COMPLEX : ALIAS_SIMPLE;
    if (find != aliases.end())
    {
        find->name = arg;
        find->replacement = repl;
        find->type = type;
        ch->sendText("Alias redefined.\r\n");
    }
    else
    {
        auto &a = aliases.emplace_back();
        a.name = arg;
        a.replacement = repl;
        // type is ALIAS_SIMPLE if repl contains no ; otherwiise it's ALIAS_COMPLEX
        a.type = type;
        ch->sendText("Alias added.\r\n");
    }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
constexpr int NUM_TOKENS = 9;

static void perform_complex_alias(struct descriptor_data *d, char *orig, struct alias_data *a)
{
    struct txt_q temp_queue;
    char *tokens[NUM_TOKENS], *temp, *write_point;
    char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH]; /* raw? */
    int num_of_tokens = 0, num;

    /* First, parse the original string */
    strcpy(buf2, orig); /* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
    temp = strtok(buf2, " ");
    while (temp && num_of_tokens < NUM_TOKENS)
    {
        tokens[num_of_tokens++] = temp;
        temp = strtok(nullptr, " ");
    }

    /* initialize */
    write_point = buf;
    temp_queue.head = temp_queue.tail = nullptr;

    /* now parse the alias */
    auto r = (char *)a->replacement.c_str();
    for (temp = r; *temp; temp++)
    {
        if (*temp == ALIAS_SEP_CHAR)
        {
            *write_point = '\0';
            buf[MAX_INPUT_LENGTH - 1] = '\0';
            write_to_q(buf, &temp_queue, 1);
            write_point = buf;
        }
        else if (*temp == ALIAS_VAR_CHAR)
        {
            temp++;
            if ((num = *temp - '1') < num_of_tokens && num >= 0)
            {
                strcpy(write_point, tokens[num]); /* strcpy: OK */
                write_point += strlen(tokens[num]);
            }
            else if (*temp == ALIAS_GLOB_CHAR)
            {
                strcpy(write_point, orig); /* strcpy: OK */
                write_point += strlen(orig);
            }
            else if ((*(write_point++) = *temp) == '$') /* redouble $ for act safety */
                *(write_point++) = '$';
        }
        else
            *(write_point++) = *temp;
    }

    *write_point = '\0';
    buf[MAX_INPUT_LENGTH - 1] = '\0';
    write_to_q(buf, &temp_queue, 1);

    /* push our temp_queue on to the _front_ of the input queue */
    for (auto q = temp_queue.head; q; q = q->next)
        d->input_queue.emplace_back(q->text);
}

/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
void perform_alias(struct descriptor_data *d, char *orig)
{
    char first_arg[MAX_INPUT_LENGTH], *ptr;
    struct alias_data *a, *tmp;

    if (!d->character)
    {
        d->input_queue.emplace_back(orig);
        return;
    }

    /* Mobs don't have alaises. */
    if (IS_NPC(d->character))
    {
        d->input_queue.emplace_back(orig);
        return;
    }
    auto &p = players.at(d->character->id);
    auto &aliases = p->aliases;

    /* bail out immediately if the guy doesn't have any aliases */
    if (aliases.empty())
    {
        d->input_queue.emplace_back(orig);
        return;
    }

    /* find the alias we're supposed to match */
    ptr = any_one_arg(orig, first_arg);

    /* bail out if it's null */
    if (!*first_arg)
    {
        d->input_queue.emplace_back(orig);
        return;
    }

    auto find = std::find_if(aliases.begin(), aliases.end(), [&](const auto &a)
                             { return boost::iequals(a.name, first_arg); });

    /* if the first arg is not an alias, return without doing anything */
    if (find == aliases.end())
    {
        d->input_queue.emplace_back(orig);
        return;
    }

    if (find->type == ALIAS_SIMPLE)
    {
        d->input_queue.emplace_back(find->replacement);
    }
    else
    {
        perform_complex_alias(d, ptr, &*find);
    }
}

/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
    int i, l;

    /*  We used to have \r as the first character on certain array items to
     *  prevent the explicit choice of that point.  It seems a bit silly to
     *  dump control characters into arrays to prevent that, so we'll just
     *  check in here to see if the first character of the argument is '!',
     *  and if so, just blindly return a '-1' for not found. - ae.
     */
    if (*arg == '!')
        return (-1);

    /* Make into lower case, and get length of string */
    for (l = 0; *(arg + l); l++)
        *(arg + l) = tolower(*(arg + l));

    if (exact)
    {
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strcmp(arg, *(list + i)))
                return (i);
    }
    else
    {
        if (!l)
            l = 1; /* Avoid "" to match the first available
                    * string */
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strncmp(arg, *(list + i), l))
                return (i);
    }

    return (-1);
}

int is_number(const char *str)
{
    while (*str)
        if (!isdigit(*(str++)))
            return (0);

    return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
    for (; **string && isspace(**string); (*string)++)
        ;
}

/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
    char *ddread, *ddwrite;

    /* If the string has no dollar signs, return immediately */
    if ((ddwrite = strchr(string, '$')) == nullptr)
        return (string);

    /* Start from the location of the first dollar sign */
    ddread = ddwrite;

    while (*ddread)                              /* Until we reach the end of the string... */
        if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
            if (*ddread == '$')
                ddread++; /* skip if we saw 2 $'s in a row */

    *ddwrite = '\0';

    return (string);
}

int fill_word(char *argument)
{
    return (search_block(argument, fill, true) >= 0);
}

void topLoad()
{
    FILE *file;
    char fname[40], line[256], filler[50];
    int x = 0;

    /* Read Toplist File */
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist"))
    {
        basic_mud_log("ERROR: Toplist file does not exist.");
        return;
    }
    else if (!(file = fopen(fname, "r")))
    {
        basic_mud_log("ERROR: Toplist file does not exist.");
        return;
    }

    TOPLOADED = true;

    while (!feof(file))
    {
        get_line(file, line);
        sscanf(line, "%s %" I64T "\n", filler, &toppoint[x]);
        topname[x] = strdup(filler);
        *filler = '\0';
        x++;
    }
    fclose(file);
}

/* Write the toplist to file */
void topWrite(Character *ch)
{
    if (GET_ADMLEVEL(ch) > 0 || IS_NPC(ch))
        return;

    if (TOPLOADED == false)
    {
        return;
    }

    char fname[40];
    FILE *fl;
    char *positions[25];
    int64_t points[25] = {0};
    int x = 0, writeEm = false, placed = false, start = 0, finish = 25, location = -1;
    int progress = false;

    if (!ch)
    {
        return;
    }

    if (!ch->desc || !GET_USER(ch))
    {
        return;
    }

    for (x = start; x < finish; x++)
    { /* Save the places as they are right now */
        positions[x] = strdup(topname[x]);
        points[x] = toppoint[x];
    }

    /* Powerlevel Section */
    /* Set the start and finish for this section */
    start = 0;
    finish = 5;

    for (x = start; x < finish; x++)
    { /* Save the new spots */
        if (placed == false)
        { /* They Haven't Placed */
            if (!boost::iequals(topname[x], GET_NAME(ch)))
            { /* Name doesn't match */
                if (GET_MAX_HIT(ch) > toppoint[x])
                {
                    free(topname[x]);
                    toppoint[x] = GET_MAX_HIT(ch);
                    topname[x] = strdup(GET_NAME(ch));
                    placed = true;
                    writeEm = true;
                    location = x;
                }
            }
            else
            { /* This is their spot already */
                placed = true;
                location = finish;
            }
        }
        else
        { /* They have placed */
            if (x < finish && location < finish)
            {
                if (!boost::iequals(positions[location], GET_NAME(ch)))
                { /* This isn't their old spot */
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
                else
                { /* This IS their old spot */
                    progress = true;
                    location += 1;
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
            }
        }
    } /* End Save New Spots*/

    if (progress == true)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the powerlevel section.@D]\r\n", GET_NAME(ch));
    }
    else if (placed == true && location != finish)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas placed in the powerlevel section.@D]\r\n", GET_NAME(ch));
    }

    location = -1;
    placed = false;
    progress = false;
    /* Ki Section         */
    /* Set the start and finish for this section */
    start = 5;
    finish = 10;

    for (x = start; x < finish; x++)
    { /* Save the new spots */
        if (placed == false)
        { /* They Haven't Placed */
            if (!boost::iequals(topname[x], GET_NAME(ch)))
            { /* Name doesn't match */
                if (GET_MAX_MANA(ch) > toppoint[x])
                {
                    free(topname[x]);
                    toppoint[x] = GET_MAX_MANA(ch);
                    topname[x] = strdup(GET_NAME(ch));
                    placed = true;
                    writeEm = true;
                    location = x;
                }
            }
            else
            { /* This is their spot already */
                placed = true;
                location = finish;
            }
        }
        else
        { /* They have placed */
            if (x < finish && location < finish)
            {
                if (!boost::iequals(positions[location], GET_NAME(ch)))
                { /* This isn't their old spot */
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
                else
                { /* This IS their old spot */
                    progress = true;
                    location += 1;
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
            }
        }
    } /* End Save New Spots*/

    if (progress == true)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the ki section.@D]\r\n", GET_NAME(ch));
    }
    else if (placed == true && location != finish)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas placed in the ki section.@D]\r\n", GET_NAME(ch));
    }

    location = -1;
    placed = false;
    progress = false;

    /* Stamina Section    */
    /* Set the start and finish for this section */
    start = 10;
    finish = 15;

    for (x = start; x < finish; x++)
    { /* Save the new spots */
        if (placed == false)
        { /* They Haven't Placed */
            if (!boost::iequals(topname[x], GET_NAME(ch)))
            { /* Name doesn't match */
                if (GET_MAX_MOVE(ch) > toppoint[x])
                {
                    free(topname[x]);
                    toppoint[x] = GET_MAX_MOVE(ch);
                    topname[x] = strdup(GET_NAME(ch));
                    placed = true;
                    writeEm = true;
                    location = x;
                }
            }
            else
            { /* This is their spot already */
                placed = true;
                location = finish;
            }
        }
        else
        { /* They have placed */
            if (x < finish && location < finish)
            {
                if (!boost::iequals(positions[location], GET_NAME(ch)))
                { /* This isn't their old spot */
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
                else
                { /* This IS their old spot */
                    progress = true;
                    location += 1;
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
            }
        }
    } /* End Save New Spots*/

    if (progress == true)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the stamina section.@D]\r\n", GET_NAME(ch));
    }
    else if (placed == true && location != finish)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas placed in the stamina section.@D]\r\n", GET_NAME(ch));
    }

    location = -1;
    placed = false;
    progress = false;

    /* Zenni Section      */
    /* Set the start and finish for this section */
    start = 15;
    finish = 20;

    for (x = start; x < finish; x++)
    { /* Save the new spots */
        if (placed == false)
        { /* They Haven't Placed */
            if (!boost::iequals(topname[x], GET_NAME(ch)))
            { /* Name doesn't match */
                if (GET_BANK_GOLD(ch) + GET_GOLD(ch) > toppoint[x])
                {
                    free(topname[x]);
                    toppoint[x] = GET_BANK_GOLD(ch) + GET_GOLD(ch);
                    topname[x] = strdup(GET_NAME(ch));
                    placed = true;
                    writeEm = true;
                    location = x;
                }
            }
            else
            { /* This is their spot already */
                placed = true;
                location = finish;
            }
        }
        else
        { /* They have placed */
            if (x < finish && location < finish)
            {
                if (!boost::iequals(positions[location], GET_NAME(ch)))
                { /* This isn't their old spot */
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
                else
                { /* This IS their old spot */
                    progress = true;
                    location += 1;
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
            }
        }
    } /* End Save New Spots*/

    if (progress == true)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the zenni section.@D]\r\n", GET_NAME(ch));
    }
    else if (placed == true && location != finish)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas placed in the zenni section.@D]\r\n", GET_NAME(ch));
    }

    location = -1;
    placed = false;
    progress = false;

    /* RPP Section        */
    /* Set the start and finish for this section */
    start = 20;
    finish = 25;

    for (x = start; x < finish; x++)
    { /* Save the new spots */
        if (placed == false)
        { /* They Haven't Placed */
            if (!boost::iequals(topname[x], GET_USER(ch)))
            { /* Name doesn't match */
                if (ch->getRPP() > toppoint[x])
                {
                    free(topname[x]);
                    toppoint[x] = ch->getRPP();
                    topname[x] = strdup(GET_USER(ch));
                    placed = true;
                    writeEm = true;
                    location = x;
                }
            }
            else
            { /* This is their spot already */
                placed = true;
                location = finish;
            }
        }
        else
        { /* They have placed */
            if (x < finish && location < finish)
            {
                if (!boost::iequals(positions[location], GET_USER(ch)))
                { /* This isn't their old spot */
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
                else
                { /* This IS their old spot */
                    progress = true;
                    location += 1;
                    free(topname[x]);
                    toppoint[x] = points[location];
                    topname[x] = strdup(positions[location]);
                    location += 1;
                }
            }
        }
    } /* End Save New Spots*/

    if (progress == true)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the RPP section.@D]\r\n", GET_USER(ch));
    }
    else if (placed == true && location != finish)
    {
        send_to_all("@D[@GToplist@W: @C%s @Whas placed in the RPP section.@D]\r\n", GET_USER(ch));
    }

    location = -1;
    placed = false;
    progress = false;

    for (x = 0; x < 25; x++)
    {
        free(positions[x]);
    }

    if (writeEm == true)
    {
        if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist"))
            return;

        if (!(fl = fopen(fname, "w")))
        {
            basic_mud_log("ERROR: could not save Toplist File, %s.", fname);
            return;
        }
        x = 0;
        while (x < 25)
        {
            fprintf(fl, "%s %" I64T "\n", topname[x], toppoint[x]);
            x++;
        }

        fclose(fl);
    }
    return;
}

int reserved_word(char *argument)
{
    return (search_block(argument, reserved, true) >= 0);
}

/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(const char *argument, char *first_arg)
{
    char *begin = first_arg;

    if (!argument)
    {
        *first_arg = '\0';
        return (nullptr);
    }

    do
    {
        skip_spaces((char**)&argument);

        first_arg = begin;
        while (*argument && !isspace(*argument))
        {
            *(first_arg++) = tolower(*argument);
            argument++;
        }

        *first_arg = '\0';
    } while (fill_word(begin));

    return (char*)argument;
}

/*
 * one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word.
 *
 * No longer ignores fill words.  -dak, 6 Jan 2003.
 */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    if (*argument == '\"')
    {
        argument++;
        while (*argument && *argument != '\"')
        {
            *(first_arg++) = tolower(*argument);
            argument++;
        }
        argument++;
    }
    else
    {
        while (*argument && !isspace(*argument))
        {
            *(first_arg++) = tolower(*argument);
            argument++;
        }
    }

    *first_arg = '\0';
    return (argument);
}

/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    while (*argument && !isspace(*argument))
    {
        *(first_arg++) = tolower(*argument);
        argument++;
    }

    *first_arg = '\0';

    return (argument);
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
    return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-);*/
}

/*
 * Same as two_arguments only, well you get the idea... - Iovan
 *
 */
char *three_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg)
{
    return (one_argument(one_argument(one_argument(argument, first_arg), second_arg), third_arg)); /* >.> */
}

/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 *
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
    if (!*arg1)
        return (0);

    for (; *arg1 && *arg2; arg1++, arg2++)
        if (tolower(*arg1) != tolower(*arg2))
            return (0);

    if (!*arg1)
        return (1);
    else
        return (0);
}

/*
 * Return first space-delimited token in arg1; remainder of string in arg2.
 *
 * NOTE: Requires sizeof(arg2) >= sizeof(string)
 */
void half_chop(char *string, char *arg1, char *arg2)
{
    char *temp;

    temp = any_one_arg(string, arg1);
    skip_spaces(&temp);
    if (arg2 != temp)
        strcpy(arg2, temp); /* strcpy: OK (documentation) */
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
    int cmd;
    for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
        if (!strcmp(complete_cmd_info[cmd].command, command))
            return (cmd);
    return (-1);
}

int special(Character *ch, int cmd, char *arg)
{
    /* special in room? */
    if (auto func = ch->location.getSpecialFunc(); func)
        if (auto r = ch->getRoom(); r && func(ch, r, cmd, arg))
            return 1;

    /* special in equipment list? */
    for (auto &[slot, obj] : ch->getEquipment())
    {
        if (auto func = GET_OBJ_SPEC(obj); func)
            if (func(ch, obj, cmd, arg))
                return 1;
    }

    /* special in inventory? */
    auto con = ch->getInventory();
    for (auto obj : filter_raw(con))
    {
        if (auto func = GET_OBJ_SPEC(obj))
            if (func(ch, obj, cmd, arg))
                return 1;
    }

    /* special in mobile present? */
    if (ch->location)
    {
        auto people = ch->location.getPeople();
        for (auto mob : filter_raw(people))
            if (IS_NPC(mob) && !MOB_FLAGGED(mob, MOB_NOTDEADYET))
                if (auto func = GET_MOB_SPEC(mob); func)
                    if (func(ch, mob, cmd, arg))
                        return 1;

        auto con = ch->location.getObjects();
        for (auto obj : filter_raw(con))
        {
            if (auto func = GET_OBJ_SPEC(obj); func)
                if (func(ch, obj, cmd, arg))
                    return 1;
        }
    }

    return 0;
}

/* *************************************************************************
 *  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
 ************************************************************************* */

/* This function needs to die. */
int _parse_name(char *arg, char *name)
{
    int i;

    skip_spaces(&arg);
    for (i = 0; (*name = *arg); arg++, i++, name++)
        if (!isalpha(*arg))
            return (1);

    if (!i)
        return (1);

    return (0);
}

constexpr int RECON = 1;
constexpr int USURP = 2;
constexpr int UNSWITCH = 3;

/* load the player, put them in the right room - used by copyover_recover too */
void enter_player_game(struct descriptor_data *d)
{

    auto ch = d->character;

    ch->timer = 0;
    reset_char(ch);

    racial_body_parts(ch);

    if (PLR_FLAGGED(ch, PLR_INVSTART))
        ch->setBaseStat("invis_level", GET_LEVEL(ch));

    ch->activate();
    if(!ch->location) {
        // They don't have a valid location. This could be because the previous
        // was deleted, or they are new.
        vnum load_room = GET_ADMLEVEL(ch) > 0 ? CONFIG_IMMORTAL_START : CONFIG_MORTAL_START;
        Location loc(load_room);
        ch->moveToLocation(loc);
    } else if(auto r = ch->getRoom()) {
        auto normalized = ch->normalizeLoadRoom(r->getVnum());
        if(normalized != r->getVnum()) {
            // The room was normalized, so we need to move the character.
            Location loc(normalized);
            ch->leaveLocation();
            ch->moveToLocation(loc);
        }
    }

    /*load_char_pets(ch);*/
    auto ac = characterSubscriptions.all("active");
    for (auto check : filter_raw(ac))
    {
        if (!check->master && IS_NPC(check) && check->getBaseStat<int>("master_id") == GET_IDNUM(ch) &&
            AFF_FLAGGED(check, AFF_CHARM) && !circle_follow(check, ch))
            add_follower(check, ch);
    }

    ch->setBaseStat("combine", -1);
    ch->setBaseStat("sleeptime", 8);
    ch->setBaseStat("food_rejuvenation", 2);
    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        ch->setBaseStat<int>("altitude", 1);
    }
    else
    {
        ch->setBaseStat<int>("altitude", 0);
    }

    for (auto f : {AFF_POSITION, AFF_SANCTUARY, AFF_ZANZOKEN})
        ch->affect_flags.set(f, false);
    ch->player_flags.set(PLR_KNOCKED, false);

    if (IS_ANDROID(ch) && !AFF_FLAGGED(ch, AFF_INFRAVISION))
    {
        ch->affect_flags.set(AFF_INFRAVISION, true);
    }

    ABSORBING(ch) = nullptr;
    ABSORBBY(ch) = nullptr;
    ch->sits.reset();
    BLOCKED(ch) = nullptr;
    BLOCKS(ch) = nullptr;
    for (const auto &s : {"spam", "rage_meter"})
        ch->setBaseStat(s, 0);
    if (!ch->affected)
    {
        ch->affect_flags.set(AFF_HEALGLOW, false);
    }
    if (AFF_FLAGGED(ch, AFF_HAYASA))
    {
        ch->setBaseStat<int>("speedboost", GET_SPEEDCALC(ch) * 0.5);
    }
    else
    {
        ch->setBaseStat<int>("speedboost", 0);
    }

    ch->player_flags.set(PLR_HEALT, false);

    if (GET_ADMLEVEL(ch) > 0)
    {
        d->level = 1;
    }

    if (IS_HOSHIJIN(ch))
    {
        if (time_info.day <= 14)
        {
            star_phase(ch, 1);
        }
        else if (time_info.day <= 21)
        {
            star_phase(ch, 2);
        }
        else
        {
            star_phase(ch, 0);
        }
    }

    if (IS_ICER(ch) && !GET_SKILL(ch, SKILL_TAILWHIP))
    {
        int numb = Random::get<int>(20, 30);
        SET_SKILL(ch, SKILL_TAILWHIP, numb);
    }
    else if (!IS_ICER(ch) && GET_SKILL(ch, SKILL_TAILWHIP))
    {
        SET_SKILL(ch, SKILL_TAILWHIP, 0);
    }

    if (ch->mutations.get(Mutation::innate_telepathy) &&
        !GET_SKILL(ch, SKILL_TELEPATHY))
    {
        SET_SKILL(ch, SKILL_TELEPATHY, 50);
    }

    if (ch->bio_genomes.get(Race::kai) &&
        !GET_SKILL(ch, SKILL_TELEPATHY) && !GET_SKILL(ch, SKILL_FOCUS))
    {
        SET_SKILL(ch, SKILL_TELEPATHY, 30);
        SET_SKILL(ch, SKILL_FOCUS, 30);
    }

    ch->setBaseStat<int>("combo", -1);
}

int readUserIndex(char *name)
{
    char fname[40];
    FILE *fl;

    /* Read User Index */
    if (!get_filename(fname, sizeof(fname), USER_FILE, name))
    {
        return 0;
    }
    else if (!(fl = fopen(fname, "r")))
    {
        return 0;
    }
    fclose(fl);
    return 1;
}

void payout(int num)
{

    struct descriptor_data *k;
    if (LASTPAYOUT == 0)
    {
        LASTPAYOUT = time(nullptr) + 86400;
        LASTPAYTYPE = num;
    }
    else if (num > LASTPAYTYPE)
    {
        LASTPAYOUT = time(nullptr) + 86400;
        LASTPAYTYPE = num;
    }
    else if (LASTPAYOUT <= time(nullptr))
    {
        LASTPAYOUT = time(nullptr) + 86400;
        LASTPAYTYPE = num;
    }
    for (k = descriptor_list; k; k = k->next)
    {
        if (GET_ADMLEVEL(k->character) <= 0 && IS_PLAYING(k) && GET_RTIME(k->character) < LASTPAYOUT)
        {
            if (num == 0)
            {
                k->account->modRPP(1);
                k->character->sendText("@D[@G+ 1 RPP@D] @cA total logon count within 4 of the highest has been achieved.@n\r\n");
            }
            else if (num == 1)
            {
                k->account->modRPP(2);
                k->character->sendText("@D[@G+ 2 RPP@D] @cThe total logon count has tied with the highest ever.@n\r\n");
            }
            else
            {
                k->account->modRPP(3);
                k->character->sendText("@D[@G+ 3 RPP@D] @cA new logon count record has been achieved!@n\r\n");
            }
            k->character->setBaseStat("rewtime", LASTPAYOUT);
        }
    }
}

int command_pass(char *cmd, Character *ch)
{

    if (AFF_FLAGGED(ch, AFF_LIQUEFIED))
    {
        if (!boost::iequals(cmd, "liquefy") && !boost::iequals(cmd, "ingest") && !boost::iequals(cmd, "look") &&
            !boost::iequals(cmd, "score") && !boost::iequals(cmd, "ooc") && !boost::iequals(cmd, "osay") && !boost::iequals(cmd, "emote") &&
            !boost::iequals(cmd, "smote") && !boost::iequals(cmd, "status"))
        {
            ch->sendText("You are not capable of performing that action while liquefied!\r\n");
            return (false);
        }
    }
    else if (IS_AFFECTED(ch, AFF_PARALYZE))
    {
        if (!boost::iequals(cmd, "look") && !boost::iequals(cmd, "score") && !boost::iequals(cmd, "ooc") && !boost::iequals(cmd, "osay") &&
            !boost::iequals(cmd, "emote") && !boost::iequals(cmd, "smote") && !boost::iequals(cmd, "status"))
        {
            ch->sendText("You are not capable of performing that action while petrified!\r\n");
            return (false);
        }
    }
    else if (IS_AFFECTED(ch, AFF_FROZEN))
    {
        if (!boost::iequals(cmd, "look") && !boost::iequals(cmd, "score") && !boost::iequals(cmd, "ooc") && !boost::iequals(cmd, "osay") &&
            !boost::iequals(cmd, "emote") && !boost::iequals(cmd, "smote") && !boost::iequals(cmd, "status"))
        {
            ch->sendText("You are not capable of performing that action while a frozen block of ice!\r\n");
            return (false);
        }
    }
    else if (IS_AFFECTED(ch, AFF_PARA) && GET_INT(ch) < Random::get<int>(1, 60))
    {
        if (!boost::iequals(cmd, "look") && !boost::iequals(cmd, "score") && !boost::iequals(cmd, "ooc") && !boost::iequals(cmd, "osay") &&
            !boost::iequals(cmd, "emote") && !boost::iequals(cmd, "smote") && !boost::iequals(cmd, "status"))
        {
            act("@yYou fail to overcome your paralysis!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@Y$n @ystruggles with $s paralysis!@n", true, ch, nullptr, nullptr, TO_ROOM);
            return (false);
        }
    }

    return (true);
}

int lockRead(char *name)
{
    char fname[40], filler[50], line[256];
    int known = false;
    FILE *fl;

    /* Read Introduction File */

    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout"))
    {
        return 0;
    }
    else if (!(fl = fopen(fname, "r")))
    {
        return 0;
    }

    while (!feof(fl))
    {
        get_line(fl, line);
        sscanf(line, "%s\n", filler);
        if (boost::iequals(CAP(name), CAP(filler)))
        {
            known = true;
        }
    }
    fclose(fl);

    if (known == true)
        return 1;
    else
        return 0;
}

/* For transfering money or doing things with an offline player */
char *rIntro(Character *ch, char *arg)
{
    char fname[40], filler[50], scrap[100], line[256];
    static char name[80];
    int known = false;
    FILE *fl;

    /* Read Introduction File */
    if (IS_NPC(ch))
    {
        return "NOTHING";
    }

    if (!get_filename(fname, sizeof(fname), INTRO_FILE, GET_NAME(ch)))
    {
        return "NOTHING";
    }
    else if (!(fl = fopen(fname, "r")))
    {
        return "NOTHING";
    }

    while (!feof(fl))
    {
        get_line(fl, line);
        sscanf(line, "%s %s\n", filler, scrap);
        if (boost::iequals(arg, scrap))
        {
            known = true;
            sprintf(name, "%s", filler);
        }
    }
    fclose(fl);

    if (known == true)
        return (name);
    else
        return "NOTHING";
}

void fingerUser(Character *ch, struct Account *account)
{
    ch->send_to("@D[@gUsername   @D: @w%-30s@D]@n\r\n", account->name.c_str());
    ch->send_to("@D[@gEmail      @D: @w%-30s@D]@n\r\n", account->email.c_str());
    ch->send_to("@D[@gTotal Slots@D: @w%-30d@D]@n\r\n", account->slots);
    ch->send_to("@D[@gRP Points  @D: @w%-30d@D]@n\r\n", account->rpp);

    if (GET_ADMLEVEL(ch) > 0)
    {
        int counter = 0;
        for (auto ref : account->characters)
        {
            auto p = players.find(ref);
            if (p == players.end())
                continue;
            ch->send_to("@D[@gCh. Slot %d @D: @w%-30s@D]@n\r\n", ++counter, p->second->character->getName());
        }
        ch->sendText("\n");
    }
}
