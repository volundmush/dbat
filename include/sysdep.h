#ifndef __SYSDEP_H__
#define __SYSDEP_H__

//#include "conf.h"
#include "typestubs.h"

#define CIRCLE_GNU_LIBC_MEMORY_TRACK	0	/* 0 = off, 1 = on */

// Some C libraires
#include <cstdio>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <strings.h>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <climits>
#include <cerrno>
#include <crypt.h>
#include <sys/time.h>
#include <ctime>
#include <cassert>
#include <zlib.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <csignal>
#include <sys/uio.h>
#include <sys/stat.h>
#include <cstddef>
#include <arpa/telnet.h>
#include <dirent.h>
#include <linux/limits.h>

#ifdef HAVE_LIBBSD
#include <bsd/string.h>
#else
#include "stringutils.h"
#endif

// C++ STD libraries
#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>
#include <memory>

// non-standard libraries
//#include "asio.hpp"
//#include "fmt/core.h"
//#include "sqlitepp.h"

/* Basic system dependencies *******************************************/
#if CIRCLE_GNU_LIBC_MEMORY_TRACK && !defined(HAVE_MCHECK_H)
#error "Cannot use GNU C library memory tracking without <mcheck.h>"
#endif

#if defined(__cplusplus)	/* C++ */
#define cpp_extern	extern
#else				/* C */
#define cpp_extern	/* Nothing */
#endif

#define HAS_RLIMIT

#define CIRCLE_UNSIGNED_INDEX 1	/* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE	uint16_t
#define NOWHERE	((IDXTYPE)~0)
#define NOTHING	((IDXTYPE)~0)
#define NOBODY	((IDXTYPE)~0)
#define NOFLAG  ((IDXTYPE)~0)
#else
#define IDXTYPE	int16_t
#define NOWHERE	(-1)	/* nil reference for rooms	*/
#define NOTHING	(-1)	/* nil reference for objects	*/
#define NOBODY		(-1)	/* nil reference for mobiles	*/
#endif

#define I64T "ld"
#define SZT "ld"
#define TMT "ld"

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;
typedef IDXTYPE obj_vnum;
typedef IDXTYPE mob_vnum;
typedef IDXTYPE zone_vnum;
typedef IDXTYPE shop_vnum;
typedef IDXTYPE trig_vnum;
typedef IDXTYPE guild_vnum;

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;
typedef IDXTYPE obj_rnum;
typedef IDXTYPE mob_rnum;
typedef IDXTYPE zone_rnum;
typedef IDXTYPE shop_rnum;
typedef IDXTYPE trig_rnum;
typedef IDXTYPE guild_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 */
typedef uint32_t bitvector_t;

#define FALSE 0
#define TRUE 1


#define ACMD(name) void (name)(struct char_data *ch, char *argument, int cmd, int subcmd)
#define SPECIAL(name) int (name)(struct char_data *ch, void *me, int cmd, char *argument)

#endif
