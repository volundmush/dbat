#include "dbat/db/descriptor.h"
#include "dbat/game/descriptor_utils.h"

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