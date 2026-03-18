/* ************************************************************************
 *   File: modify.c                                      Part of CircleMUD *
 *  Usage: Run-time modification of game variables                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/modify.hpp"
#include "dbat/game/send.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/db.hpp"
//#include "dbat/game/comm.hpp"
#include "dbat/game/spells.hpp"
#include "dbat/game/mail.hpp"
#include "dbat/game/improved-edit.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/spell_parser.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/game/utils.hpp"

#include "dbat/game/const/AdminLevel.hpp"

/* local functions */

static void smash_numb(char *str);

static void playing_string_cleanup(struct descriptor_data *d, int action);

static void exdesc_string_cleanup(struct descriptor_data *d, int action);

// std::string editor helper functions
static void std_smash_tilde(std::string &str);
static void std_smash_numb(std::string &str);
static int std_improved_editor_execute(struct descriptor_data *d, const std::string &str);
static std::vector<std::string> std_split_lines(const std::string &text);
static std::string std_join_lines(const std::vector<std::string> &lines);
static void std_parse_help(struct descriptor_data *d);
static void std_parse_delete(const std::string &args, struct descriptor_data *d);
static void std_parse_list_norm(const std::string &args, struct descriptor_data *d);
static void std_parse_list_num(const std::string &args, struct descriptor_data *d);
static void std_parse_insert(const std::string &args, struct descriptor_data *d);
static void std_parse_edit(const std::string &args, struct descriptor_data *d);
static void std_parse_replace(const std::string &args, struct descriptor_data *d);
static void std_parse_format(const std::string &args, struct descriptor_data *d);

/* maximum length for text field x+1 */
static const int length[] =
    {
        15,
        60,
        256,
        240,
        60};

/* ************************************************************************
 *  modification of malloc'ed strings                                      *
 ************************************************************************ */

/*
 * Put '#if 1' here to erase ~, or roll your own method.  A common idea
 * is smash/show tilde to convert the tilde to another innocuous character
 * to save and then back to display it. Whatever you do, at least keep the
 * function around because other MUD packages use it, like mudFTP.
 *   -gg 9/9/98
 */
void smash_tilde(char *str)
{
    /*
     * Erase any _line ending_ tildes inserted in the editor.
     * The load mechanism can't handle those, yet.
     * -- Welcor 04/2003
     */

    char *p = str;
    for (; *p; p++)
        if (*p == '~' && (*(p + 1) == '\r' || *(p + 1) == '\n' || *(p + 1) == '\0'))
            *p = ' ';
#if 1
    /*
     * Erase any ~'s inserted by people in the editor.  This prevents anyone
     * using online creation from causing parse errors in the world files.
     * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
     * his tildes thank you very much.), -gg 2/20/98
     */
    while ((str = strchr(str, '~')))
        *str = ' ';
#endif
}

static void smash_numb(char *str)
{
    /*
     * Erase any _line ending_ tildes inserted in the editor.
     * The load mechanism can't handle those, yet.
     * -- Welcor 04/2003
     */

    char *p = str;
    for (; *p; p++)
        if (*p == '#' && (*(p + 1) == '\r' || *(p + 1) == '\n' || *(p + 1) == '\0'))
            *p = ' ';
#if 1
    /*
     * Erase any ~'s inserted by people in the editor.  This prevents anyone
     * using online creation from causing parse errors in the world files.
     * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
     * his tildes thank you very much.), -gg 2/20/98
     */
    while ((str = strchr(str, '#')))
        *str = ' ';
#endif
}

void string_write(struct descriptor_data *d, std::string *writeto, size_t len, long mailto, std::string backup)
{
    if (d->character && !IS_NPC(d->character))
        d->character->player_flags.set(PLR_WRITING, true);

    d->std_backstr = backup;
    d->str = nullptr;
    d->std_str = writeto;
    d->max_str = len;
    d->mail_to = mailto;
}


// Helper functions for std::string editor

// std::string version of smash_tilde
static void std_smash_tilde(std::string &str)
{
    // Replace line-ending tildes
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '~' && (i + 1 >= str.length() || str[i + 1] == '\r' || str[i + 1] == '\n'))
        {
            str[i] = ' ';
        }
    }
    // Replace all tildes
    boost::algorithm::replace_all(str, "~", " ");
}

// std::string version of smash_numb
static void std_smash_numb(std::string &str)
{
    // Replace line-ending hashes
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '#' && (i + 1 >= str.length() || str[i + 1] == '\r' || str[i + 1] == '\n'))
        {
            str[i] = ' ';
        }
    }
    // Replace all hashes
    boost::algorithm::replace_all(str, "#", " ");
}

// std::string version of improved editor execute
static int std_improved_editor_execute(struct descriptor_data *d, const std::string &str)
{
    if (!using_improved_editor || str.empty() || str[0] != '/')
    {
        return STRINGADD_OK;
    }

    std::string actions;
    if (str.length() > 2)
    {
        actions = str.substr(2);
    }

    char command = (str.length() > 1) ? str[1] : '\0';

    switch (command)
    {
    case 'a':
        return STRINGADD_ABORT;
    case 'c':
        if (!d->std_str->empty())
        {
            d->std_str->clear();
            d->sendText("Current buffer cleared.\r\n");
        }
        else
        {
            d->sendText("Current buffer empty.\r\n");
        }
        break;
    case 'd':
        std_parse_delete(actions, d);
        break;
    case 'e':
        std_parse_edit(actions, d);
        break;
    case 'f':
        if (!d->std_str->empty())
        {
            std_parse_format(actions, d);
        }
        else
        {
            d->sendText("Current buffer empty.\r\n");
        }
        break;
    case 'i':
        if (!d->std_str->empty())
        {
            std_parse_insert(actions, d);
        }
        else
        {
            d->sendText("Current buffer empty.\r\n");
        }
        break;
    case 'h':
        std_parse_help(d);
        break;
    case 'l':
        if (!d->std_str->empty())
        {
            std_parse_list_norm(actions, d);
        }
        else
        {
            d->sendText("Current buffer empty.\r\n");
        }
        break;
    case 'n':
        if (!d->std_str->empty())
        {
            std_parse_list_num(actions, d);
        }
        else
        {
            d->sendText("Current buffer empty.\r\n");
        }
        break;
    case 'r':
        std_parse_replace(actions, d);
        break;
    case 's':
        return STRINGADD_SAVE;
    default:
        d->sendText("Invalid option.\r\n");
        break;
    }
    return STRINGADD_ACTION;
}

// Helper to split string into lines and get specific line ranges
static std::vector<std::string> std_split_lines(const std::string &text)
{
    std::vector<std::string> lines;
    boost::algorithm::split(lines, text, boost::algorithm::is_any_of("\n"), boost::algorithm::token_compress_off);
    return lines;
}

// Helper to join lines back into string
static std::string std_join_lines(const std::vector<std::string> &lines)
{
    return boost::algorithm::join(lines, "\n");
}

// std::string version of parse_help
static void std_parse_help(struct descriptor_data *d)
{
    d->sendText(
        "Editor command formats: /<letter>\r\n\r\n"
        "/a         -  aborts editor\r\n"
        "/c         -  clears buffer\r\n"
        "/d#        -  deletes a line #\r\n"
        "/e# <text> -  changes the line at # with <text>\r\n"
        "/f         -  formats text\r\n"
        "/fi        -  indented formatting of text\r\n"
        "/fi#       -  indented formatting on a specific line\r\n"
        "/fi #-#    -  indented formatting on specific lines\r\n"
        "/h         -  list text editor commands\r\n"
        "/i# <text> -  inserts <text> before line #\r\n"
        "/l         -  lists buffer\r\n"
        "/n         -  lists buffer with line numbers\r\n"
        "/r 'a' 'b' -  replace 1st occurrence of text <a> in buffer with text <b>\r\n"
        "/ra 'a' 'b'-  replace all occurrences of text <a> within buffer with text <b>\r\n"
        "              usage: /r[a] 'pattern' 'replacement'\r\n"
        "/s         -  saves text\r\n");
}

// std::string version of parse_delete
static void std_parse_delete(const std::string &args, struct descriptor_data *d)
{
    if (d->std_str->empty())
    {
        d->sendText("Buffer is empty.\r\n");
        return;
    }

    int line_low = 0, line_high = 0;
    int parsed = sscanf(args.c_str(), " %d - %d ", &line_low, &line_high);

    if (parsed == 0)
    {
        d->sendText("You must specify a line number or range to delete.\r\n");
        return;
    }
    else if (parsed == 1)
    {
        line_high = line_low;
    }
    else if (line_high < line_low)
    {
        d->sendText("That range is invalid.\r\n");
        return;
    }

    if (line_low <= 0)
    {
        d->sendText("Invalid, line numbers to delete must be higher than 0.\r\n");
        return;
    }

    auto lines = std_split_lines(*d->std_str);

    // Convert to 0-based indexing
    line_low--;
    line_high--;

    if (line_low >= static_cast<int>(lines.size()))
    {
        d->sendText("Line(s) out of range; not deleting.\r\n");
        return;
    }

    line_high = std::min(line_high, static_cast<int>(lines.size()) - 1);
    int deleted_count = line_high - line_low + 1;

    lines.erase(lines.begin() + line_low, lines.begin() + line_high + 1);
    *d->std_str = std_join_lines(lines);

    d->send_to("%d line%s deleted.\r\n", deleted_count, (deleted_count != 1 ? "s" : ""));
}

// std::string version of parse_list_norm
static void std_parse_list_norm(const std::string &args, struct descriptor_data *d)
{
    int line_low = 1, line_high = 999999;

    if (!args.empty())
    {
        int parsed = sscanf(args.c_str(), " %d - %d ", &line_low, &line_high);
        if (parsed == 1)
        {
            line_high = line_low;
        }
    }

    if (line_low < 1)
    {
        d->sendText("Line numbers must be greater than 0.\r\n");
        return;
    }
    else if (line_high < line_low)
    {
        d->sendText("That range is invalid.\r\n");
        return;
    }

    auto lines = std_split_lines(*d->std_str);

    // Convert to 0-based indexing
    line_low--;
    line_high--;

    if (line_low >= static_cast<int>(lines.size()))
    {
        d->sendText("Line(s) out of range; no buffer listing.\r\n");
        return;
    }

    line_high = std::min(line_high, static_cast<int>(lines.size()) - 1);

    std::string output;
    if (line_high < 999998 || line_low > 0)
    {
        output = fmt::format("Current buffer range [%d - %d]:\r\n", line_low + 1, line_high + 1);
    }

    for (int i = line_low; i <= line_high; ++i)
    {
        output += lines[i] + "\r\n";
    }

    int shown_count = line_high - line_low + 1;
    output += fmt::format("\r\n%d line%s shown.\r\n", shown_count, (shown_count != 1) ? "s" : "");

    d->sendText(output);
}

// std::string version of parse_list_num
static void std_parse_list_num(const std::string &args, struct descriptor_data *d)
{
    int line_low = 1, line_high = 999999;

    if (!args.empty())
    {
        int parsed = sscanf(args.c_str(), " %d - %d ", &line_low, &line_high);
        if (parsed == 1)
        {
            line_high = line_low;
        }
    }

    if (line_low < 1)
    {
        d->sendText("Line numbers must be greater than 0.\r\n");
        return;
    }
    else if (line_high < line_low)
    {
        d->sendText("That range is invalid.\r\n");
        return;
    }

    auto lines = std_split_lines(*d->std_str);

    // Convert to 0-based indexing
    line_low--;
    line_high--;

    if (line_low >= static_cast<int>(lines.size()))
    {
        d->sendText("Line(s) out of range; no buffer listing.\r\n");
        return;
    }

    line_high = std::min(line_high, static_cast<int>(lines.size()) - 1);

    std::string output;
    for (int i = line_low; i <= line_high; ++i)
    {
        output += fmt::format("{:4d}: {}\r\n", i + 1, lines[i]);
    }

    d->sendText(output);
}

// std::string version of parse_insert
static void std_parse_insert(const std::string &args, struct descriptor_data *d)
{
    if (d->std_str->empty())
    {
        d->sendText("Buffer is empty, nowhere to insert.\r\n");
        return;
    }

    std::istringstream iss(args);
    std::string line_str, text;
    iss >> line_str;
    std::getline(iss, text);

    if (line_str.empty())
    {
        d->sendText("You must specify a line number before which to insert text.\r\n");
        return;
    }

    int line_num = std::stoi(line_str);
    if (line_num <= 0)
    {
        d->sendText("Line number must be higher than 0.\r\n");
        return;
    }

    // Trim leading space from text
    if (!text.empty() && text[0] == ' ')
    {
        text = text.substr(1);
    }

    auto lines = std_split_lines(*d->std_str);

    // Convert to 0-based indexing
    line_num--;

    if (line_num > static_cast<int>(lines.size()))
    {
        d->sendText("Line number out of range; insert aborted.\r\n");
        return;
    }

    // Check if adding this line would exceed max_str
    std::string new_content = std_join_lines(lines);
    if (new_content.length() + text.length() + 3 > d->max_str)
    {
        d->sendText("Insert text pushes buffer over maximum size, insert aborted.\r\n");
        return;
    }

    lines.insert(lines.begin() + line_num, text);
    *d->std_str = std_join_lines(lines);

    d->sendText("Line inserted.\r\n");
}

// std::string version of parse_edit
static void std_parse_edit(const std::string &args, struct descriptor_data *d)
{
    if (d->std_str->empty())
    {
        d->sendText("Buffer is empty, nothing to change.\r\n");
        return;
    }

    std::istringstream iss(args);
    std::string line_str, text;
    iss >> line_str;
    std::getline(iss, text);

    if (line_str.empty())
    {
        d->sendText("You must specify a line number at which to change text.\r\n");
        return;
    }

    int line_num = std::stoi(line_str);
    if (line_num <= 0)
    {
        d->sendText("Line number must be higher than 0.\r\n");
        return;
    }

    // Trim leading space from text
    if (!text.empty() && text[0] == ' ')
    {
        text = text.substr(1);
    }

    auto lines = std_split_lines(*d->std_str);

    // Convert to 0-based indexing
    line_num--;

    if (line_num >= static_cast<int>(lines.size()))
    {
        d->sendText("Line number out of range; change aborted.\r\n");
        return;
    }

    // Replace the line
    lines[line_num] = text;
    std::string new_content = std_join_lines(lines);

    if (new_content.length() > d->max_str)
    {
        d->sendText("Change causes new length to exceed buffer maximum size, aborted.\r\n");
        return;
    }

    *d->std_str = new_content;
    d->sendText("Line changed.\r\n");
}

// std::string version of parse_replace
static void std_parse_replace(const std::string &args, struct descriptor_data *d)
{
    if (d->std_str->empty())
    {
        return;
    }

    bool rep_all = false;
    std::string working_args = args;

    // Check for 'a' flag
    if (!working_args.empty() && std::isalpha(working_args[0]))
    {
        if (working_args[0] == 'a')
        {
            rep_all = true;
        }
        working_args = working_args.substr(1);
    }

    // Parse the pattern and replacement using simple string parsing
    size_t first_quote = working_args.find('\'');
    if (first_quote == std::string::npos)
    {
        d->sendText("Invalid format.\r\n");
        return;
    }

    size_t second_quote = working_args.find('\'', first_quote + 1);
    if (second_quote == std::string::npos)
    {
        d->sendText("Target string must be enclosed in single quotes.\r\n");
        return;
    }

    size_t third_quote = working_args.find('\'', second_quote + 1);
    if (third_quote == std::string::npos)
    {
        d->sendText("No replacement string.\r\n");
        return;
    }

    size_t fourth_quote = working_args.find('\'', third_quote + 1);
    if (fourth_quote == std::string::npos)
    {
        d->sendText("Replacement string must be enclosed in single quotes.\r\n");
        return;
    }

    std::string pattern = working_args.substr(first_quote + 1, second_quote - first_quote - 1);
    std::string replacement = working_args.substr(third_quote + 1, fourth_quote - third_quote - 1);

    // Check if replacement would make string too long
    size_t pattern_count = 0;
    std::string temp_str = *d->std_str;
    size_t pos = 0;
    while ((pos = temp_str.find(pattern, pos)) != std::string::npos)
    {
        pattern_count++;
        pos += pattern.length();
        if (!rep_all)
            break;
    }

    if (pattern_count == 0)
    {
        d->send_to("String '%s' not found.\r\n", pattern.c_str());
        return;
    }

    size_t new_length = d->std_str->length() - (pattern_count * pattern.length()) + (pattern_count * replacement.length());
    if (new_length > d->max_str)
    {
        d->sendText("Not enough space left in buffer.\r\n");
        return;
    }

    // Perform the replacement
    if (rep_all)
    {
        boost::algorithm::replace_all(*d->std_str, pattern, replacement);
    }
    else
    {
        boost::algorithm::replace_first(*d->std_str, pattern, replacement);
    }

    d->send_to("Replaced %d occurrence%s of '%s' with '%s'.\r\n",
               static_cast<int>(pattern_count), (pattern_count != 1) ? "s" : "", pattern.c_str(), replacement.c_str());
}

// std::string version of parse_format (simplified)
static void std_parse_format(const std::string &args, struct descriptor_data *d)
{
    // For now, just a simple implementation that doesn't change formatting
    // The original format_text function is quite complex and handles word wrapping
    // This can be enhanced later if needed
    d->sendText("Text formatting not yet implemented for std::string editor.\r\n");
}

void std_string_add(struct descriptor_data *d, char *str)
{
    int action;

    delete_doubledollar(str);
    std::string input_str(str);
    std_smash_tilde(input_str);
    std_smash_numb(input_str);

    // Check for terminal string (@)
    if (input_str == "@")
    {
        action = STRINGADD_SAVE;
        input_str.clear();
    }
    else if ((action = std_improved_editor_execute(d, input_str)) == STRINGADD_ACTION)
    {
        return;
    }
    else
    {
        action = STRINGADD_OK;
    }

    if (action == STRINGADD_OK)
    {
        if (d->std_str->empty())
        {
            if (input_str.length() + 3 > d->max_str)
            { // \r\n\0
                d->character->sendText("String too long - Truncated.\r\n");
                input_str = input_str.substr(0, d->max_str - 3);
                *d->std_str = input_str;
                if (!using_improved_editor)
                {
                    action = STRINGADD_SAVE;
                }
            }
            else
            {
                *d->std_str = input_str;
            }
        }
        else
        {
            if (input_str.length() + d->std_str->length() + 3 > d->max_str)
            { // \r\n\0
                d->character->sendText("String too long. Last line skipped.\r\n");
                if (!using_improved_editor)
                {
                    action = STRINGADD_SAVE;
                }
                else if (action == STRINGADD_OK)
                {
                    action = STRINGADD_ACTION; // No appending \r\n, but still let them save
                }
            }
            else
            {
                if (!d->std_str->empty())
                {
                    *d->std_str += "\r\n";
                }
                *d->std_str += input_str;
            }
        }
    }

    // Common cleanup code adapted from normal string_add...
    switch (action)
    {
    case STRINGADD_ABORT:
        switch (STATE(d))
        {
        case CON_CEDIT:
        case CON_TEDIT:
        case CON_NEWSEDIT:
        case CON_REDIT:
        case CON_MEDIT:
        case CON_OEDIT:
        case CON_IEDIT:
        case CON_EXDESC:
        case CON_TRIGEDIT:
        case CON_HEDIT:
            *d->std_str = d->std_backstr;
            break;
        case CON_PLAYING:
            /* all CON_PLAYING are handled below in playing_string_cleanup */
            break;

        default:
            basic_mud_log("SYSERR: string_add: Aborting write from unknown origin.");
            break;
        }
        break;
    case STRINGADD_SAVE:
        if (d->std_str && d->std_str->empty())
        {
            *d->std_str = "Nothing.\r\n";
        }
        break;
    case STRINGADD_ACTION:
        break;
    }

    /* Ok, now final cleanup. */

    if (action == STRINGADD_SAVE || action == STRINGADD_ABORT)
    {

        /* Common post cleanup code. */
        d->str = nullptr;
        d->std_str = nullptr;
        d->mail_to = 0;
        d->max_str = 0;
        if (d->character && !IS_NPC(d->character))
        {
            for (auto f : {PLR_MAILING, PLR_WRITING})
                d->character->player_flags.set(f, false);
        }
    }
    else if (action != STRINGADD_ACTION && d->std_str->size() + 3 <= d->max_str) /* 3 = \r\n\0 */
        *d->std_str += "\r\n";
}

/*
 * Add user input to the 'current' string (as defined by d->str).
 * This is still overly complex.
 */
void string_add(struct descriptor_data *d, char *str)
{
    int action;

    delete_doubledollar(str);
    smash_tilde(str);
    smash_numb(str);

    /* determine if this is the terminal string, and truncate if so */
    /* changed to only accept '@' at the beginning of line - J. Elson 1/17/94 */
    /* changed to only accept '@' if it's by itself - fnord 10/15/2004 */
    if ((action = (*str == '@' && !str[1])))
        *str = '\0';
    else if ((action = improved_editor_execute(d, str)) == STRINGADD_ACTION)
        return;

    if (action != STRINGADD_OK)
        /* Do nothing. */;
    else if (!(*d->str))
    {
        if (strlen(str) + 3 > d->max_str)
        { /* \r\n\0 */
            d->character->sendText("String too long - Truncated.\r\n");
            strcpy(str + (d->max_str - 3), "\r\n");
            CREATE(*d->str, char, d->max_str);
            strcpy(*d->str, str); /* strcpy: OK (size checked) */
            if (!using_improved_editor)
                action = STRINGADD_SAVE;
        }
        else
        {
            CREATE(*d->str, char, strlen(str) + 3);
            strcpy(*d->str, str); /* strcpy: OK (size checked) */
        }
    }
    else
    {
        if (strlen(str) + strlen(*d->str) + 3 > d->max_str)
        { /* \r\n\0 */
            d->character->sendText("String too long.  Last line skipped.\r\n");
            if (!using_improved_editor)
                action = STRINGADD_SAVE;
            else if (action == STRINGADD_OK)
                action = STRINGADD_ACTION; /* No appending \r\n\0, but still let them save. */
        }
        else
        {
            RECREATE(*d->str, char, strlen(*d->str) + strlen(str) + 3); /* \r\n\0 */
            strcat(*d->str, str);                                       /* strcat: OK (size precalculated) */
        }
    }

    /*
     * Common cleanup code.
     */
    switch (action)
    {
    case STRINGADD_ABORT:
        switch (STATE(d))
        {
        case CON_CEDIT:
        case CON_TEDIT:
        case CON_NEWSEDIT:
        case CON_REDIT:
        case CON_MEDIT:
        case CON_OEDIT:
        case CON_IEDIT:
        case CON_EXDESC:
        case CON_TRIGEDIT:
        case CON_HEDIT:
            free(*d->str);
            *d->str = d->backstr;
            d->backstr = nullptr;
            d->str = nullptr;
            break;
        case CON_PLAYING:
            /* all CON_PLAYING are handled below in playing_string_cleanup */
            break;

        default:
            basic_mud_log("SYSERR: string_add: Aborting write from unknown origin.");
            break;
        }
        break;
    case STRINGADD_SAVE:
        if (d->str && *d->str && **d->str == '\0')
        {
            free(*d->str);
            *d->str = strdup("Nothing.\r\n");
        }
        if (d->backstr)
            free(d->backstr);
        d->backstr = nullptr;
        break;
    case STRINGADD_ACTION:
        break;
    }

    /* Ok, now final cleanup. */

    if (action == STRINGADD_SAVE || action == STRINGADD_ABORT)
    {

        /* Common post cleanup code. */
        d->str = nullptr;
        d->mail_to = 0;
        d->max_str = 0;
        if (d->character && !IS_NPC(d->character))
        {
            for (auto f : {PLR_MAILING, PLR_WRITING})
                d->character->player_flags.set(f, false);
        }
    }
    else if (action != STRINGADD_ACTION && strlen(*d->str) + 3 <= d->max_str) /* 3 = \r\n\0 */
        strcat(*d->str, "\r\n");
}

static void playing_string_cleanup(struct descriptor_data *d, int action)
{
    struct board_info *board;
    struct board_msg *fore, *cur, *aft;

    if (PLR_FLAGGED(d->character, PLR_MAILING))
    {
        if (action == STRINGADD_SAVE && *d->str)
        {
            store_mail(d->mail_to, GET_IDNUM(d->character), *d->str);
            d->sendText("Message sent!\r\n");
            notify_if_playing(d->character, d->mail_to);
        }
        else
        {
            d->sendText("Mail aborted.\r\n");
            free(*d->str);
            free(d->str);
        }
    }

    if (PLR_FLAGGED(d->character, PLR_WRITING))
    {
        if (d->mail_to >= BOARD_MAGIC)
        {
            if (action == STRINGADD_ABORT)
            {
                /* find the message */
                board = locate_board(d->mail_to - BOARD_MAGIC);
                fore = cur = aft = nullptr;
                for (cur = BOARD_MESSAGES(board); cur; cur = aft)
                {
                    aft = MESG_NEXT(cur);
                    if (cur->data == *d->str)
                    {
                        if (BOARD_MESSAGES(board) == cur)
                        {
                            if (MESG_NEXT(cur))
                            {
                                BOARD_MESSAGES(board) = MESG_NEXT(cur);
                            }
                            else
                            {
                                BOARD_MESSAGES(board) = nullptr;
                            }
                        }
                        if (fore)
                        {
                            MESG_NEXT(fore) = aft;
                        }
                        if (aft)
                        {
                            MESG_PREV(aft) = fore;
                        }
                        free(cur->subject);
                        free(cur->data);
                        free(cur);
                        BOARD_MNUM(board)
                        --;
                        d->sendText("Post aborted.\r\n");
                        return;
                    }
                    fore = cur;
                }
                d->sendText("Unable to find your message to delete it!\r\n");
            }
            else
            {
                d->sendText("\r\nPost saved.\r\n");
                save_board(locate_board(d->mail_to - BOARD_MAGIC));
            }
        }

        /* hm... I wonder what happens when you can't finish writing a note */
    }
}

static void exdesc_string_cleanup(struct descriptor_data *d, int action)
{
    if (action == STRINGADD_ABORT)
        d->sendText("Description aborted.\r\n");

    STATE(d) = CON_PLAYING;
}

/* *********************************************************************
 *  Modification of character skills                                 *
 ********************************************************************* */

ACMD(do_skillset)
{
    Character *vict;
    char name[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH], help[MAX_STRING_LENGTH];
    int skill, value, i = 0, qend;

    argument = one_argument(argument, name);

    if (!*name)
    { /* no arguments. print an informative text */
        ch->sendText("Syntax: skillset <name> '<skill>' <value>\r\n"
                     "Skill being one of the following:\r\n");
        for (qend = 0, i = 0; i < SKILL_TABLE_SIZE; i++)
        {
            if (spell_info[i].name == unused_spellname) /* This is valid. */
                continue;
            ch->send_to("%18s", spell_info[i].name);
            if (qend++ % 4 == 3)
                ch->sendText("\r\n");
        }
        if (qend % 4 != 0)
            ch->sendText("\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
    {
        ch->send_to("%s", CONFIG_NOPERSON);
        return;
    }
    skip_spaces(&argument);

    /* If there is no chars in argument */
    if (!*argument)
    {
        i = snprintf(help, sizeof(help) - i, "\r\nSkills:\r\n");
        i += print_skills_by_type(vict, help + i, sizeof(help) - i, SKTYPE_SKILL, nullptr);
        i += snprintf(help + i, sizeof(help) - i, "\r\nSpells:\r\n");
        i += print_skills_by_type(vict, help + i, sizeof(help) - i, SKTYPE_SPELL, nullptr);
        if (CONFIG_ENABLE_LANGUAGES)
        {
            i += snprintf(help + i, sizeof(help) - i, "\r\nLanguages:\r\n");
            i += print_skills_by_type(vict, help + i, sizeof(help) - i, SKTYPE_SKILL | SKTYPE_LANG, nullptr);
        }
        if (i >= sizeof(help))
            strcpy(help + sizeof(help) - strlen("** OVERFLOW **") - 1, "** OVERFLOW **"); /* strcpy: OK */
        ch->desc->send_to("%s", help);
        return;
    }

    if (*argument != '\'')
    {
        ch->sendText("Skill must be enclosed in: ''\r\n");
        return;
    }
    /* Locate the last quote and lowercase the magic words (if any) */

    for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
        argument[qend] = tolower(argument[qend]);

    if (argument[qend] != '\'')
    {
        ch->sendText("Skill must be enclosed in: ''\r\n");
        return;
    }
    strcpy(help, (argument + 1)); /* strcpy: OK (MAX_INPUT_LENGTH <= MAX_STRING_LENGTH) */
    help[qend - 1] = '\0';
    if ((skill = find_skill_num(help, SKTYPE_SKILL)) <= 0)
    {
        ch->sendText("Unrecognized skill.\r\n");
        return;
    }
    argument += qend + 1; /* skip to next parameter */
    argument = one_argument(argument, buf);

    if (!*buf)
    {
        ch->sendText("Learned value expected.\r\n");
        return;
    }
    value = atoi(buf);
    if (value < 0)
    {
        ch->sendText("Minimum value for learned is 0.\r\n");
        return;
    }

    /*
     * find_skill_num() guarantees a valid spell_info[] index, or -1, and we
     * checked for the -1 above so we are safe here.
     */
    SET_SKILL(vict, skill, value);
    mudlog(BRF, ADMLVL_IMMORT, true, "skillset: %s changed %s's '%s' to %d.", GET_NAME(ch), GET_NAME(vict),
           spell_info[skill].name, value);
    ch->send_to("You change %s's %s to %d.\r\n", GET_NAME(vict), spell_info[skill].name, value);
}
