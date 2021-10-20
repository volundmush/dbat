/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __COMM_H__
#define __COMM_H__


#include "conf.h"
#include "sysdep.h"

#if CIRCLE_GNU_LIBC_MEMORY_TRACK
# include <mcheck.h>
#endif

#ifdef CIRCLE_MACINTOSH		/* Includes for the Macintosh */
# define SIGPIPE 13
# define SIGALRM 14
  /* GUSI headers */
# include <sys/ioctl.h>
  /* Codewarrior dependant */
# include <SIOUX.h>
# include <console.h>
#endif

#ifdef CIRCLE_WINDOWS		/* Includes for Win32 */
# ifdef __BORLANDC__
#  include <dir.h>
# else /* MSVC */
#  include <direct.h>
# endif
# include <mmsystem.h>
#endif /* CIRCLE_WINDOWS */

#ifdef CIRCLE_AMIGA		/* Includes for the Amiga */
# include <sys/ioctl.h>
# include <clib/socket_protos.h>
#endif /* CIRCLE_AMIGA */

#ifdef CIRCLE_ACORN		/* Includes for the Acorn (RiscOS) */
# include <socklib.h>
# include <inetlib.h>
# include <sys/ioctl.h>
#endif

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "oasis.h"
#include "genolc.h"
#include "dg_scripts.h"
#include "dg_event.h"
#include "screen.h"
#include "imc.h"
#include "maputils.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

/* mccp defines */
#define COMPRESS2 86

#define NUM_RESERVED_DESCS	8
#define COPYOVER_FILE "copyover.dat"

/* comm.c */
size_t	send_to_char(struct char_data *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_all(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void	send_to_room(room_rnum room, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_outdoor(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void    send_to_moon(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void    send_to_planet(int type, int planet, const char *messg, ...) __attribute__ ((format (printf, 3, 4)));
void	close_socket(struct descriptor_data *d);
void	send_to_range(room_vnum start, room_vnum finish, const char *messg, ...) __attribute__ ((format (printf, 3, 4)));
void	perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to);
char	*act(const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, int type);

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define TO_GMOTE	5
#define TO_SLEEP        (2 << 7)	/* to char, even if sleeping */
#define DG_NO_TRIG      (2 << 8)	/* don't check act trigger   */
#define TO_SNEAKRESIST  (2 << 9)	/* resisted sneaking roll    */
#define TO_HIDERESIST   (2 << 10)	/* resisted hiding roll      */

/* I/O functions */
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
int	write_to_descriptor(socket_t desc, const char *txt, struct compr *comp);
size_t	write_to_output(struct descriptor_data *d, const char *txt, ...) __attribute__ ((format (printf, 2, 3)));
size_t	vwrite_to_output(struct descriptor_data *d, const char *format, va_list args);
void	string_add(struct descriptor_data *d, char *str);
void	string_write(struct descriptor_data *d, char **txt, size_t len, long mailto, void *data);

#define PAGE_LENGTH	22
#define PAGE_WIDTH	79
void	page_string(struct descriptor_data *d, char *str, int keep_internal);

typedef RETSIGTYPE sigfunc(int);
void   show_help(struct descriptor_data *t, const char *entry);

/* variables */
extern unsigned long pulse;
extern FILE *logfile;

#endif