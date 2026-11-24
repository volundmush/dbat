#pragma once
#include <cstdarg>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>

#include "dbat/consts/roomflags.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/format.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/net/platform.hpp"

constexpr int NUM_RESERVED_DESCS     = 8;
constexpr int MAX_CONNECTIONS_PER_IP = 10;  /* simultaneous connections from one address */
constexpr int MAX_QUEUED_INPUT       = 100; /* typed commands waiting to run before a client counts as flooding */
#define COPYOVER_FILE "copyover.dat"

/* messages.cpp */
int arena_watch(GameWorld *g, struct char_data *ch);

/* send_to_room(): the text as it is */
void send_to_room(GameWorld *g, room_vnum room, std::string_view text);
/* send_to_room(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_room(GameWorld *g, room_vnum room, std::format_string<Args...> fmt, Args &&...args)
{
  send_to_room(g, room, str_format(fmt, std::forward<Args>(args)...));
}

/* send_to_all(): the text as it is */
void send_to_all(GameWorld *g, std::string_view text);
/* send_to_all(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_all(GameWorld *g, std::format_string<Args...> fmt, Args &&...args)
{
  send_to_all(g, str_format(fmt, std::forward<Args>(args)...));
}

/* send_to_outdoor(): the text as it is */
void send_to_outdoor(GameWorld *g, std::string_view text);
/* send_to_outdoor(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_outdoor(GameWorld *g, std::format_string<Args...> fmt, Args &&...args)
{
  send_to_outdoor(g, str_format(fmt, std::forward<Args>(args)...));
}

/* send_to_moon(): the text as it is */
void send_to_moon(GameWorld *g, std::string_view text);
/* send_to_moon(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_moon(GameWorld *g, std::format_string<Args...> fmt, Args &&...args)
{
  send_to_moon(g, str_format(fmt, std::forward<Args>(args)...));
}

/* send_to_planet(): the text as it is */
void send_to_planet(GameWorld *g, int type, room_flag planet, std::string_view text);
/* send_to_planet(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_planet(GameWorld *g, int type, room_flag planet, std::format_string<Args...> fmt, Args &&...args)
{
  send_to_planet(g, type, planet, str_format(fmt, std::forward<Args>(args)...));
}

/* send_to_range(): the text as it is */
void send_to_range(GameWorld *g, room_vnum start, room_vnum finish, std::string_view text);
/* send_to_range(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_range(GameWorld *g, room_vnum start, room_vnum finish, std::format_string<Args...> fmt, Args &&...args)
{
  send_to_range(g, start, finish, str_format(fmt, std::forward<Args>(args)...));
}
/* Sends str, with its $ codes filled in, to those type names. Nothing is sent for an empty str. The message made,
   or null if none has been made yet (it is the last one made, even when none was sent this time: a quirk). */
const std::string *act(GameWorld *g, std::string_view str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, int type);
/* No char pointer for act()'s message, which could be null: a std::string, a view or a literal */
template <class P>
  requires(!std::is_array_v<std::remove_reference_t<P>> && std::is_pointer_v<std::decay_t<P>> &&
           std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::decay_t<P>>>, char>)
void act(GameWorld *g, P &&str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, int type) = delete;
/* The three messages of an act() at a victim: to the actor, to the victim and to everyone else, and act()'s
 * hide_invisible for them. An empty one isn't sent. */
struct act_msgs {
  std::string_view to_char{};
  std::string_view to_vict{};
  std::string_view to_room{};
  bool hide{};
};
/* act() of each of m in turn: to ch, to vict, then to the rest of the room */
void act_triple(GameWorld *g, const act_msgs &m, struct char_data *ch, struct obj_data *obj, struct char_data *vict);
/* ... or to ch, to the rest of the room, then to vict */
void act_triple_vict_last(GameWorld *g, const act_msgs &m, struct char_data *ch, struct obj_data *obj, struct char_data *vict);
void close_socket(GameWorld *g, struct descriptor_data *d);
void close_all_sockets(GameWorld *g);

constexpr int TO_ROOM        = 1;
constexpr int TO_VICT        = 2;
constexpr int TO_NOTVICT     = 3;
constexpr int TO_CHAR        = 4;
constexpr int TO_GMOTE       = 5;
constexpr int TO_SLEEP       = (2 << 7);  /* to char, even if sleeping */
constexpr int DG_NO_TRIG     = (2 << 8);  /* don't check act trigger   */
constexpr int TO_SNEAKRESIST = (2 << 9);  /* resisted sneaking roll    */
constexpr int TO_HIDERESIST  = (2 << 10); /* resisted hiding roll      */

/* I/O functions */
void write_to_q(std::string_view txt, struct txt_q *queue, int aliased);
int write_to_descriptor(net::native_handle desc, std::string_view txt);
/* Queues text for d as it is */
size_t write_to_output(GameWorld *g, struct descriptor_data *d, std::string_view text);
/* Queues text for d, formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
size_t write_to_output(GameWorld *g, struct descriptor_data *d, std::format_string<Args...> fmt, Args &&...args)
{
  return write_to_output(g, d, str_format(fmt, std::forward<Args>(args)...));
}
void string_add(GameWorld *g, struct descriptor_data *d, std::string_view typed);
void string_write(struct descriptor_data *d, std::string *txt, size_t len, long mailto);
/* Starts the editor on target with what is there now, without string_write()'s player flags */
void string_edit_begin(struct descriptor_data *d, std::string *target, size_t len);

constexpr int PAGE_WIDTH = 79; /* column at which the text editor wraps lines */

void show_help(GameWorld *g, struct descriptor_data *t, std::string_view entry);

/* variables */
extern FILE *logfile;

// functions
void init_game(GameWorld *g, uint16_t port);

/* The parts of one game loop pass that need no sockets. */
int game_add_connection(GameWorld *g, net::native_handle desc, std::string_view host);
int game_read_input(GameWorld *g, struct descriptor_data *d);
int game_send_output(GameWorld *g, struct descriptor_data *d);
void game_process_commands(GameWorld *g);
void game_finish_descriptors(GameWorld *g);
void game_heartbeat(GameWorld *g, int heart_pulse);

/* Sends the log to filename (stderr reopened on it; appended to if keep_old_log, as after a copyover), or a default
   file if that can't be opened, or stderr if filename is empty: exits if none of the files can be opened */
void setup_log(std::string_view filename, bool keep_old_log);
bool sendok(struct char_data *ch, int to_sleeping);
