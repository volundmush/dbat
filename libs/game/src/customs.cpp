#include "dbat/game/customs.h"
void customWrite(struct char_data *ch, struct obj_data *obj)
{

 if (IS_NPC(ch))
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
