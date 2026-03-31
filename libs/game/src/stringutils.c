//
// Created by basti on 10/22/2021.
//

#ifndef HAVE_LIBBSD
#include "dbat/game/stringutils.h"
#include <string.h>
#include "dbat/db/characters.h"
#include "dbat/db/objects.h"
#include "dbat/db/rooms.h"

size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}

#endif