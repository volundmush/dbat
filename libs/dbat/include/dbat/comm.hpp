#pragma once
#include <cstdarg>
#include <cstdio>
#include <string>
#include <string_view>

#include <sys/socket.h>

#include "dbat/consts/types.hpp"
#include "dbat/format.hpp"
#include "dbat/descriptor.hpp"

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

constexpr int INVALID_SOCKET = -1;

/* mccp defines */
constexpr int COMPRESS2 = 86;

constexpr int NUM_RESERVED_DESCS     = 8;
constexpr int MAX_CONNECTIONS_PER_IP = 10;  /* simultaneous connections from one address */
constexpr int MAX_QUEUED_INPUT       = 100; /* typed commands waiting to run before a client counts as flooding */
#define COPYOVER_FILE "copyover.dat"

/* comm.c */
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
void send_to_planet(GameWorld *g, int type, int planet, std::string_view text);
/* send_to_planet(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_planet(GameWorld *g, int type, int planet, std::format_string<Args...> fmt, Args &&...args)
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
char *act(GameWorld *g, const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, int type);
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
void write_to_q(const char *txt, struct txt_q *queue, int aliased);
int write_to_descriptor(int desc, const char *txt);
/* Queues text for d as it is */
size_t write_to_output(GameWorld *g, struct descriptor_data *d, std::string_view text);
/* Queues text for d, formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
size_t write_to_output(GameWorld *g, struct descriptor_data *d, std::format_string<Args...> fmt, Args &&...args)
{
  return write_to_output(g, d, str_format(fmt, std::forward<Args>(args)...));
}
void string_add(GameWorld *g, struct descriptor_data *d, char *str);
void string_write(struct descriptor_data *d, std::string *txt, size_t len, long mailto);
/* Starts the editor on target with what is there now, without string_write()'s player flags */
void string_edit_begin(struct descriptor_data *d, std::string *target, size_t len);

constexpr int PAGE_WIDTH = 79; /* column at which the text editor wraps lines */

void show_help(GameWorld *g, struct descriptor_data *t, const char *entry);

/* variables */
extern FILE *logfile;
extern const char compress_offer[4];

// functions
void init_game(GameWorld *g, uint16_t port);

/* The parts of one game loop pass that need no sockets. */
int game_add_connection(GameWorld *g, int desc, const char *host);
int game_read_input(GameWorld *g, struct descriptor_data *d);
int game_send_output(GameWorld *g, struct descriptor_data *d);
void game_process_commands(GameWorld *g);
void game_finish_descriptors(GameWorld *g);
void game_heartbeat(GameWorld *g, int heart_pulse);

void free_bufpool(GameWorld *g);
void setup_log(const char *filename, int fd, bool keep_old_log);
bool sendok(struct char_data *ch, int to_sleeping);

