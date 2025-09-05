#pragma once
#include <cstdint>
#include <map>
#include <unordered_map>
#include <string>
#include <list>

#include "Log.h"
#include "Typedefs.h"

#include "const/ConnectionState.h"
#include "const/Max.h"

struct Object;
struct Character;
struct Account;

struct txt_block {
    char *text;
    int aliased;
    struct txt_block *next;
};

struct txt_q {
    struct txt_block *head;
    struct txt_block *tail;
};

struct descriptor_data {
    int64_t id{NOTHING};
    std::unordered_map<int64_t, std::string> conns;
    void onConnectionLost(int64_t connId);
    void onConnectionClosed(int64_t connId);

    char host[HOST_LENGTH + 1];    /* hostname				*/
    int connected{CON_PLAYING};        /* mode of 'connectedness'		*/

    time_t login_time{time(nullptr)};        /* when the person connected		*/
    std::string* std_str{nullptr}; // for the alternate modify-str system...
    std::string std_backstr{};    // for the alternate modify-str system...
    char **str{};            /* for the modify-str system		*/
    char *backstr{};        /* backup string for modify-str system	*/
    size_t max_str{};            /* maximum size of string in modify-str	*/
    int32_t mail_to{};        /* name for mail system			*/
    bool has_prompt{true};        /* is the user at a prompt?             */
    std::string last_input;        /* the last input			*/
    std::list<std::string> raw_input_queue, input_queue;
    std::string output;        /* ptr to the current output buffer	*/
    std::string processed_output;
    std::list<std::string> history;        /* History of commands, for ! mostly.	*/
    struct Character *character{};    /* linked to char			*/
    struct Character *original{};    /* original char if switched		*/
    struct descriptor_data *snooping{}; /* Who is this char snooping	*/
    struct descriptor_data *snoop_by{}; /* And who is snooping this char	*/
    struct descriptor_data *next{}; /* link to next descriptor		*/
    struct Account *account{}; /* Account info                        */
    int level{};
    char *newsbuf{};
    /*---------------Player Level Object Editing Variables-------------------*/
    int obj_editval{};
    int obj_editflag;
    char *obj_was{};
    char *obj_name{};
    char *obj_short{};
    char *obj_long{};
    int obj_type{};
    int obj_weapon{};
    struct Object *obj_point{};
    char *title{};
    double timeoutCounter{0};
    void handle_input();
    void start();
    void handleLostLastConnection(bool graceful);
    void sendText(std::string_view txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in descriptor_data::sendFmt: %s", e.what());
            LERROR("Template was: %s", format.data());
        }
    }

    template<typename... Args>
    size_t send_to(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
            if(formatted_string.empty()) return 0;
            sendText(formatted_string);
            return formatted_string.size();
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in descriptor_data::send_to: %s", e.what());
            LERROR("Template was: %s", format.data());
            return 0;
        }
    }
};

#define IS_PLAYING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT ||      \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||       \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT ||       \
                        STATE(d) == CON_CEDIT || STATE(d) == CON_PLAYING ||     \
                        STATE(d) == CON_TRIGEDIT || STATE(d) == CON_AEDIT ||    \
                        STATE(d) == CON_GEDIT || STATE(d) == CON_IEDIT ||       \
                        STATE(d) == CON_HEDIT || STATE(d) == CON_NEWSEDIT ||    \
                        STATE(d) == CON_POBJ)
#define IS_INMENU(d)    (STATE(d) == CON_MENU || STATE(d) == CON_EXDESC || STATE(d) == CON_UMENU || STATE(d) == CON_GET_USER || STATE(d) == CON_GET_EMAIL || STATE(d) == CON_CHPWD_GETOLD || STATE(d) == CON_CHPWD_GETNEW || STATE(d) == CON_CHPWD_VRFY || STATE(d) == CON_DELCNF1 || STATE(d) == CON_DELCNF2 || STATE(d) == CON_QRACE || STATE(d) == CON_QCLASS || STATE(d) == CON_CLASS_HELP || STATE(d) == CON_RACE_HELP || STATE(d) == CON_BONUS || STATE(d) == CON_NEGATIVE || STATE(d) == CON_DISTFEA || STATE(d) == CON_HW || STATE(d) == CON_AURA)
#define STATE(d)    ((d)->connected)

extern struct descriptor_data *descriptor_list;
extern std::map<int64_t, struct descriptor_data *> sessions;