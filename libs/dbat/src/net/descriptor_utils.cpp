#include "dbat/descriptor.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/db.hpp"
#include "dbat/fileop.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/comm.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

void customWrite(struct char_data *ch, struct obj_data *obj)
{
  struct atomic_file af;

  if (char_is_npc(ch))
    return;

  char fname[40], line[READ_SIZE], prev[READ_SIZE] = "";
  FILE *fl, *file;

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, char_get_desc(ch)->user.c_str())) {
    mud_log("ERROR: Custom unable to be saved to user file!");
    return;
  }

  if (!(fl = atomic_open(&af, fname, true))) {
    mud_log("ERROR: Custom unable to be saved to user file!");
    return;
  }

  /* copy the existing list (if there is one yet), dropping repeated lines, then add the new item */
  if ((file = fopen(fname, "r"))) {
    while (get_line(file, line)) {
      if (strcasecmp(prev, line))
        fprintf(fl, "%s\n", line);
      strlcpy(prev, line, sizeof(prev));
    }
    fclose(file);
  }

  fprintf(fl, "%s\n\n", obj_get_short_description(obj));

  atomic_commit(&af);
}

void customRead(GameWorld *g, struct descriptor_data *d, int type, char *name)
{

  char fname[40], line[256], filler[256] = "";
  FILE *fl;
  char buf[MAX_STRING_LENGTH] = "";

  if (type == 1) {

    if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, name)) {
      mud_log("ERROR: Custom unable to be read from user file!");
      return;
    }

    if (!(fl = fopen(fname, "r"))) {
      mud_log("ERROR: Custom file unable to be read!");
      return;
    }

    while (!feof(fl)) {
      get_line(fl, line);
      if (strcasecmp(filler, line))
        sprintf(buf + strlen(buf), "%s\n", line);
      *filler = '\0';
      *line   = '\0';
      sprintf(filler, "%s", line);
    }

    send_to_char(g, d->character, "%s", buf);

    fclose(fl);
    return;
  }

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, d->user.c_str())) {
    mud_log("ERROR: Custom unable to be read from user file!");
    return;
  }

  if (!(fl = fopen(fname, "r"))) {
    mud_log("ERROR: Custom file unable to be read!");
    return;
  }

  while (!feof(fl)) {
    get_line(fl, line);
    if (strcasecmp(filler, line))
      sprintf(buf + strlen(buf), "%s\n", line);
    *filler = '\0';
    sprintf(filler, "%s", line);
  }

  write_to_output(g, d, "%s", buf);

  fclose(fl);
}

void customCreate(struct descriptor_data *d)
{
  struct atomic_file af;

  if (!d)
    return;

  if (d->customfile == 1)
    return;

  char fname[40];
  FILE *fl;

  /* Create Their Custom File */

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, d->user.c_str()))
    return;

  if (!(fl = atomic_open(&af, fname, true))) {
    mud_log("ERROR: could not create custom file.");
    return;
  }

  fprintf(fl, "@D--@RUser @GC@gustom@Gs@D--@n\n");
  d->customfile = 1;

  atomic_commit(&af);
}

/* Formerly macros in descriptor_utils.hpp. */

bool desc_is_playing(const struct descriptor_data *d)
{
  return (desc_get_connected(d) == CON_TEDIT || desc_get_connected(d) == CON_REDIT || desc_get_connected(d) == CON_MEDIT || desc_get_connected(d) == CON_OEDIT || desc_get_connected(d) == CON_ZEDIT || desc_get_connected(d) == CON_SEDIT || desc_get_connected(d) == CON_CEDIT || desc_get_connected(d) == CON_PLAYING || desc_get_connected(d) == CON_TRIGEDIT || desc_get_connected(d) == CON_AEDIT || desc_get_connected(d) == CON_GEDIT || desc_get_connected(d) == CON_IEDIT || desc_get_connected(d) == CON_HEDIT || desc_get_connected(d) == CON_NEWSEDIT || desc_get_connected(d) == CON_POBJ || desc_get_connected(d) == CON_ASSEDIT || desc_get_connected(d) == CON_HSEDIT || desc_get_connected(d) == CON_LEVELUP);
}

/* Field accessors (formerly accessor macros). */

int desc_get_connected(const struct descriptor_data *d)
{
  return d->connected;
}

void desc_set_connected(struct descriptor_data *d, int v)
{
  d->connected = v;
}
