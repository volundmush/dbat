/*************************************************************************
*   File: connection.cpp                                Part of CircleMUD *
*  Usage: Sockets and connections: listening, accepting, closing        *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"

#include "comm_internal.hpp"

static struct in_addr *get_bind_addr(GameWorld *g);
static void nonblock(int s);
static int set_sendbuf(int s);

/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(GameWorld *g, uint16_t cmport)
{
  int s;
  struct sockaddr_in sa;
  int opt;

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("SYSERR: Error creating socket");
    exit(1);
  }

  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }

  set_sendbuf(s);

  {
    struct linger ld;

    ld.l_onoff  = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0)
      perror("SYSERR: setsockopt SO_LINGER"); /* Not fatal I suppose. */
  }

  /* Clear the structure */
  memset((char *)&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port   = htons(cmport);
  sa.sin_addr   = *(get_bind_addr(g));

  if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    close(s);
    exit(1);
  }
  nonblock(s);
  if (listen(s, SOMAXCONN) < 0) {
    perror("SYSERR: listen");
    close(s);
    exit(1);
  }
  return (s);
}

int get_max_players(GameWorld *g)
{

  int max_descs = 0;
  const char *how;

  /*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
  {
    struct rlimit limit;

    /* find the limit of file descs */
    how = "rlimit";
    if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling getrlimit");
      exit(1);
    }

    /* set the current to the maximum */
    limit.rlim_cur = limit.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling setrlimit");
      exit(1);
    }
    if (limit.rlim_max == RLIM_INFINITY)
      max_descs = CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS;
    else
      max_descs = (int)MIN((rlim_t)(CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS), limit.rlim_max);
  }

  /* now calculate max _players_ based on max descs */
  max_descs = MIN(CONFIG_MAX_PLAYING, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    mud_log("SYSERR: Non-positive max player limit!  (Set at {:d} using {}).",
            max_descs, how);
    exit(1);
  }
  mud_log("   Setting player limit to {:d} using {}.", max_descs, how);
  return (max_descs);
}

/* Empty the queues before closing connection */
static void flush_queues(GameWorld *g, struct descriptor_data *d)
{
  if (d->large_outbuf) {
    d->large_outbuf->next = g->bufpool;
    g->bufpool               = d->large_outbuf;
    d->large_outbuf       = nullptr;
    d->output             = d->small_outbuf;
  }
  flush_input_queue(d);
}

/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */

/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

static struct in_addr *get_bind_addr(GameWorld *g)
{
  static struct in_addr bind_addr;

  /* Clear the structure */
  memset((char *)&bind_addr, 0, sizeof(bind_addr));

  /* If DLFT_IP is unspecified, use INADDR_ANY */
  if (CONFIG_DFLT_IP.empty()) {
    bind_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* If the parsing fails, use INADDR_ANY */
    if (!inet_aton(CONFIG_DFLT_IP.c_str(), &bind_addr)) {
      mud_log("SYSERR: DFLT_IP of {} appears to be an invalid IP address",
              CONFIG_DFLT_IP);
      bind_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  /* Put the address that we've finally decided on into the logs */
  if (bind_addr.s_addr == htonl(INADDR_ANY))
    mud_log("Binding to all IP interfaces on this host.");
  else
    mud_log("Binding only to IP address {}", inet_ntoa(bind_addr));

  return (&bind_addr);
}

/* Sets the kernel's send buffer size for the descriptor */
static int set_sendbuf(int s)
{
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return (-1);
  }

  return (0);
}

/* Initialize a descriptor */
void init_descriptor(GameWorld *g, struct descriptor_data *newd, int desc)
{

  newd->descriptor = desc;
  newd->output     = newd->small_outbuf;
  newd->bufspace   = SMALL_BUFSIZE - 1;
  newd->login_time = game_now(g);
  newd->has_prompt = 1; /* prompt is part of greetings */
  desc_set_connected(newd, CON_GET_USER);
  /* Numbers run from 1 to 999 and wrap, skipping any still in use. */
  do {
    if (++g->last_desc == 1000)
      g->last_desc = 1;
  } while (g->descriptors.find(g->last_desc) && g->descriptors.size() < 999);
  newd->desc_num = g->last_desc;
}

struct descriptor_data *add_descriptor(GameWorld *g, int socket, std::string_view host)
{
  auto *d = new descriptor_data();
  snprintf(d->host, sizeof(d->host), "%.*s", (int)host.size(), host.data());
  init_descriptor(g, d, socket);
  g->descriptors.add(d->desc_num, d);
  return d;
}

std::string_view desc_output(const struct descriptor_data *d) { return d->output ? d->output : ""; }

void desc_discard_output(GameWorld *g, struct descriptor_data *d)
{
  if (d->large_outbuf) {
    d->large_outbuf->next = g->bufpool;
    g->bufpool            = d->large_outbuf;
    d->large_outbuf       = nullptr;
  }
  d->output   = d->small_outbuf;
  d->bufspace = SMALL_BUFSIZE - 1;
  d->bufptr   = 0;
  *d->output  = '\0';
}

void desc_rewrite_output(struct descriptor_data *d, std::string_view text)
{
  /* quirk: measured, as bufptr can be off (process_output() leaves it negative after saving an unsent prompt) */
  const int shrunk = (int)(strlen(d->output) - text.size());
  memcpy(d->output, text.data(), text.size());
  d->output[text.size()] = '\0';
  d->bufspace += shrunk;
  d->bufptr -= shrunk;
}

int desc_socket(const struct descriptor_data *d) { return d->descriptor; }

/* What a connection holds before it has logged in to an account. */
void set_empty_account(struct descriptor_data *d)
{
  d->acct.user  = "Empty";
  d->acct.pass  = "Empty";
  d->acct.email = "Empty";
  d->acct.slots.fill("Empty");
}

static void set_color(GameWorld *g, struct descriptor_data *d)
{
  if (d->character == nullptr) {
    d->character = new char_data();
    clear_char(d->character);
    char_make_player_specials(d->character);
    char_set_desc(d->character, d);
  }
  char_prf_set(g, d->character, PRF_COLOR, true);
  write_to_output(g, d, g->GREETANSI);
  write_to_output(g, d, "\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
  write_to_output(g, d, "@D                 ---(@CPeak Logon Count Today@W: @w{:>4d}@D)---@n\r\n", g->PCOUNT);
  write_to_output(g, d, "@D                 ---(@CHighest Logon Count   @W: @w{:>4d}@D)---@n\r\n", g->HIGHPCOUNT);
  write_to_output(g, d, "@D                 ---(@CTotal Era {:d} Characters@W: @w{:>4}@D)---@n\r\n", CURRENT_ERA, add_commas(g->ERAPLAYERS));
  write_to_output(g, d, "\r\n@cEnter your desired username or the username you have already made.\n@CEnter Username:@n\r\n");
  set_empty_account(d);
  return;
}

int new_descriptor(GameWorld *g, int s)
{
  int desc;
  socklen_t i;
  char host[HOST_LENGTH + 1];
  struct sockaddr_in peer;
  int keepalive = 1;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *)&peer, &i)) == INVALID_SOCKET) {
    perror("SYSERR: accept");
    return (-1);
  }
  /* keep it from blocking */
  nonblock(desc);

  /* set the send buffer size */
  if (set_sendbuf(desc) < 0) {
    close(desc);
    return (0);
  }
  /* notice connections that silently went away (NAT timeouts, crashed clients) */
  if (setsockopt(desc, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0)
    perror("SYSERR: setsockopt SO_KEEPALIVE");

  /* The site is the numeric address. No reverse DNS: the lookup blocks the whole game, and the name it
     returns is chosen by whoever controls the address, so it is no basis for bans. */
  if (!inet_ntop(AF_INET, &peer.sin_addr, host, sizeof(host)))
    strcpy(host, "unknown");

  return game_add_connection(g, desc, host);
}

/* Takes on a connected socket, unless the game is full or the site isn't welcome. */
int game_add_connection(GameWorld *g, int desc, const char *host)
{
  int sockets_connected = 0, from_same_ip = 0;
  struct descriptor_data *newd;

  /* make sure we have room for it */
  sockets_connected += count_members(each_desc(g));

  if (sockets_connected >= CONFIG_MAX_PLAYING) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... please try again later!\r\n");
    close(desc);
    return (0);
  }
  /* create a new descriptor */
  newd = new descriptor_data();

  snprintf(newd->host, sizeof(newd->host), "%s", host);

  /* one host may not take all the slots */
  for (descriptor_data *other : each_desc(g))
    if (!strcmp(other->host, newd->host))
      from_same_ip++;
  if (from_same_ip >= MAX_CONNECTIONS_PER_IP) {
    write_to_descriptor(desc, "Too many connections from your address. Please close some and try again.\r\n");
    close(desc);
    mudlog(g, LOG_CMP, ADMLVL_GOD, true, "Connection from [{}] refused: already {:d} connections from it", newd->host, from_same_ip);
    delete newd;
    return (0);
  }

  /* determine if the site is banned */
  if (isbanned(g, newd->host) == BAN_ALL) {
    close(desc);
    mudlog(g, LOG_CMP, ADMLVL_GOD, true, "Connection attempt denied from [{}]", newd->host);
    delete newd;
    return (0);
  }

  /* initialize descriptor data */
  init_descriptor(g, newd, desc);

  /* prepend to list */
  g->descriptors.add(newd->desc_num, newd);

  set_color(g, newd);

  return (0);
}

/* Frees the account strings: real ones after login, or the "Empty" placeholders set_color() made. */
static void free_user(struct descriptor_data *d)
{
  if (d->user_freed == 1) {
    return;
  }
  d->user_freed = 1;

  if (!d->acct.user.empty() && strcasecmp(d->acct.user.c_str(), "Empty"))
    mud_log("Freeing User: {}", d->acct.user);

  for (auto &slot : d->acct.slots)
    slot.clear();
  d->acct.user.clear();
  d->acct.pass.clear();
  d->acct.email.clear();
}

void close_all_sockets(GameWorld *g)
{
  for (descriptor_data *d : each_desc(g))
    close_socket(g, d);
}

void close_socket(GameWorld *g, struct descriptor_data *d)
{
  g->descriptors.remove(d->desc_num, d);
  close(d->descriptor);
  flush_queues(g, d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = nullptr;

  if (d->snoop_by) {
    write_to_output(g, d->snoop_by, "Your victim is no longer among us.\r\n");
    d->snoop_by->snooping = nullptr;
  }

  if (d->character) {
    /* If we're switched, this resets the mobile taken. */
    char_set_desc(d->character, nullptr);

    /* Plug memory leak, from Eric Green. */
    if (!char_is_npc(d->character) && char_plr_flagged(d->character, PLR_MAILING) && d->str) {
      d->str = nullptr; /* the letter being written, in d->draft */
    }
    if (desc_is_playing(d) || desc_get_connected(d) == CON_DISCONNECT) {
      struct char_data *link_challenged = d->original ? d->original : d->character;

      /* We are guaranteed to have a person. */
      act(g, "$n has lost $s link.", true, link_challenged, nullptr, nullptr, TO_ROOM);
      save_char(g, link_challenged);
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, link_challenged)), true, "Closing link to: {}.", char_get_name(link_challenged));
    } else {
      free_char(g, d->character);
      d->character = nullptr; /* (free_char() can't clear it: ->desc was cut above) */
    }
  } else
    mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "Losing descriptor without char.");

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && char_get_desc(d->original))
    char_set_desc(d->original, nullptr);

  free_user(d);

  /*. Kill any OLC stuff (every editor, and character creation's stat points, keep theirs in d->olc) .*/
  leave_olc(g, d);

  delete d;
}

void check_idle_passwords(GameWorld *g)
{

  for (descriptor_data *d : each_desc(g)) {
    if (desc_get_connected(d) != CON_PASSWORD && desc_get_connected(d) != CON_GET_EMAIL && desc_get_connected(d) != CON_NEWPASSWD)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    }
    write_to_output(g, d, "\r\nTimed out... goodbye.\r\n");
    desc_set_connected(d, CON_CLOSE);
  }
}

void check_idle_menu(GameWorld *g)
{

  for (descriptor_data *d : each_desc(g)) {
    if (desc_get_connected(d) != CON_MENU && desc_get_connected(d) != CON_GET_USER && desc_get_connected(d) != CON_UMENU)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      write_to_output(g, d, "\r\nYou are about to be disconnected due to inactivity in 60 seconds.\r\n");
      continue;
    }
    write_to_output(g, d, "\r\nTimed out... goodbye.\r\n");
    desc_set_connected(d, CON_CLOSE);
  }
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

static void nonblock(int s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}
