#include "dbat/descriptor.h"
#include "dbat/descriptor_utils.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/db.h"
#include "dbat/fileop.h"
#include "dbat/stringutils.h"
#include "dbat/comm.h"

void customWrite(struct char_data *ch, struct obj_data *obj)
{

 if (char_is_npc(ch))
  return;

 char fname[40], line[256], prev[256];
 char buf[MAX_STRING_LENGTH];
 FILE *fl, *file;

 if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, ch->desc->user)) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 if (!(file = fopen(fname, "r"))) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 while (!feof(file)) {
  get_line(file, line);
  if (strcasecmp(prev, line))
   sprintf(buf+strlen(buf), "%s\n", line);
  *prev = '\0';
  sprintf(prev, line);
 }

 fclose(file);

 if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, ch->desc->user)) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 if (!(fl = fopen(fname, "w"))) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 sprintf(buf+strlen(buf), "%s\n", obj->short_description);
 fprintf(fl, "%s\n", buf);
 

 fclose(fl);
}


void customRead(struct descriptor_data *d, int type, char *name)
{

 char fname[40], line[256], filler[256];
 FILE *fl;
 char buf[MAX_STRING_LENGTH];

 if (type == 1) {

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, name)) {
   log("ERROR: Custom unable to be read from user file!");
   return;
  }

  if (!(fl = fopen(fname, "r"))) {
   log("ERROR: Custom file unable to be read!");
   return;
  }

  char buf[MAX_STRING_LENGTH];

  while (!feof(fl)) {
   get_line(fl, line);
   if (strcasecmp(filler, line))
    sprintf(buf+strlen(buf), "%s\n", line);
   *filler = '\0';
   *line = '\0';
   sprintf(filler, line);
  }

  send_to_char(d->character, buf);

  fclose(fl);    
  return;
 } else {

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, d->user)) {
   log("ERROR: Custom unable to be read from user file!");
   return;
  }

  if (!(fl = fopen(fname, "r"))) {
   log("ERROR: Custom file unable to be read!");
   return;
  }

  while (!feof(fl)) {
   get_line(fl, line);
   if (strcasecmp(filler, line))
    sprintf(buf+strlen(buf), "%s\n", line);
   *filler = '\0';
   sprintf(filler, line);
  }

  write_to_output(d, buf);

  fclose(fl);
 }
}

void customCreate(struct descriptor_data *d)
{

  if (!d)
   return;

  if (d->customfile == 1)
   return;

  char fname[40];
  FILE *fl;

  /* Create Their Custom File */

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, d->user))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not create custom file.");
    return;
  }

 fprintf(fl, "@D--@RUser @GC@gustom@Gs@D--@n\n");
 d->customfile = 1;

 fclose(fl);
}

/* Formerly macros in descriptor_utils.h. */

bool desc_is_playing(const struct descriptor_data *d) {
    return (desc_get_connected(d) == CON_TEDIT
           || desc_get_connected(d) == CON_REDIT
           || desc_get_connected(d) == CON_MEDIT
           || desc_get_connected(d) == CON_OEDIT
           || desc_get_connected(d) == CON_ZEDIT
           || desc_get_connected(d) == CON_SEDIT
           || desc_get_connected(d) == CON_CEDIT
           || desc_get_connected(d) == CON_PLAYING
           || desc_get_connected(d) == CON_TRIGEDIT
           || desc_get_connected(d) == CON_AEDIT
           || desc_get_connected(d) == CON_GEDIT
           || desc_get_connected(d) == CON_IEDIT
           || desc_get_connected(d) == CON_HEDIT
           || desc_get_connected(d) == CON_NEWSEDIT
           || desc_get_connected(d) == CON_POBJ);
}

/* Field accessors (formerly accessor macros). */

int desc_get_connected(const struct descriptor_data *d) {
    return d->connected;
}

void desc_set_connected(struct descriptor_data *d, int v) {
    d->connected = v;
}
