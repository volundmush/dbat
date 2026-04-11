#include "dbat/game/fileop.h"
#include "dbat/db/utils.h"
#include "dbat/game/utils.h"
#include "dbat/game/stringutils.h"
#include "dbat/db/consts/maximums.h"
#include "dbat/game/log.h"
#include "dbat/game/db.h"
#include "dbat/game/comm.h"
#include "dbat/game/config.h"

#include <linux/limits.h>
#include <errno.h>

/* get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large.  */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}


int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  case NEW_OBJ_FILES:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case PLR_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PLR;
    break;
  case PET_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PET;
    break;
  case USER_FILE:
    prefix = LIB_USER;
    suffix = SUF_USER;
    break;
  case INTRO_FILE:
    prefix = LIB_INTRO;
    suffix = SUF_INTRO;
    break;
  case SENSE_FILE:
    prefix = LIB_SENSE;
    suffix = SUF_SENSE;
    break;
  case CUSTOME_FILE:
    prefix = LIB_USER;
    suffix = SUF_CUSTOM;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
  return (1);
}

/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}



void topLoad()
{
  FILE *file;
  char fname[40], line[256], filler[50];
  int x = 0;

  /* Read Toplist File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist")) {
    log("ERROR: Toplist file does not exist.");
    return;
  }
  else if (!(file = fopen(fname, "r"))) {
    log("ERROR: Toplist file does not exist.");
    return;
  }

 
  TOPLOADED = TRUE;

  while (!feof(file)) {
    get_line(file, line);
    sscanf(line, "%s %" I64T "\n", filler, &toppoint[x]);
    topname[x] = strdup(filler);
    *filler = '\0';
    x++;
  }
  fclose(file);
}

/* Write the toplist to file */
void topWrite(struct char_data *ch)
{
  if (GET_ADMLEVEL(ch) > 0 || IS_NPC(ch))
   return;

  if (TOPLOADED == FALSE) {
   return;
  }

  char fname[40];
  FILE *fl;
  char *positions[25];
  int64_t points[25] = {0};
  int x = 0, writeEm = FALSE, placed = FALSE, start = 0, finish = 25, location = -1;
  int progress = FALSE;

  if (!ch) {
   return;
  }

  if (!ch->desc || !GET_USER(ch)) {
   return;
  }

  for (x = start; x < finish; x++) { /* Save the places as they are right now */
   positions[x] = strdup(topname[x]);
   points[x] = toppoint[x];
  }

  /* Powerlevel Section */
   /* Set the start and finish for this section */
    start = 0;
    finish = 5;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_MAX_HIT(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_MAX_HIT(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the powerlevel section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the powerlevel section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;
  /* Ki Section         */
   /* Set the start and finish for this section */
    start = 5;
    finish = 10;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_MAX_MANA(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_MAX_MANA(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the ki section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the ki section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  /* Stamina Section    */
   /* Set the start and finish for this section */
    start = 10;
    finish = 15;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_MAX_MOVE(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_MAX_MOVE(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the stamina section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the stamina section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  /* Zenni Section      */
   /* Set the start and finish for this section */
    start = 15;
    finish = 20;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_BANK_GOLD(ch) + GET_GOLD(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_BANK_GOLD(ch) + GET_GOLD(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the zenni section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the zenni section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  /* RPP Section        */
   /* Set the start and finish for this section */
    start = 20;
    finish = 25;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_USER(ch))) { /* Name doesn't match */
      if (GET_TRP(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_TRP(ch);
       topname[x] = strdup(GET_USER(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_USER(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the RPP section.@D]\r\n", GET_USER(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the RPP section.@D]\r\n", GET_USER(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  for(x = 0; x < 25; x++) {
   free(positions[x]);
  }

  if (writeEm == TRUE) {
   if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist"))
     return;

   if( !(fl = fopen(fname, "w")) ) {
     log("ERROR: could not save Toplist File, %s.", fname);
     return;
   }
    x = 0;
    while(x < 25) {
     fprintf(fl, "%s %" I64T "\n", topname[x], toppoint[x]);
     x++;
    }

   fclose(fl);
  }
  return;
}