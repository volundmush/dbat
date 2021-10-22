//********************************************************************************
//
// This is the implementation for clans on Aeonian Dreams.
//
// Clans can be created in order for players to create their own rules to
// follow within some scope (i.e. the clan). For instance, there could be a
// PKiller's clan that allows all members to kill each other, or a Roleplayer's
// clan where all members agree to roleplay whenever they are in the presence
// of another member.
//
// Clans were implemented as a curiousity to see how players handle a world
// where they create and govern their own rules. Call me a sadist, but I'm
// interested in what will happen...
//
// If this isn't your cup of tea, Clans can easily act as a fairly decent
// method for implementing clans. And, unlike most clan code out there, any given
// player can belong to any number of clans. If you decide to use our clan
// code, please make note of it in your helpfile(s) relating to clans. Simply
// noting that this was orionally written by Alister of Aeonian Dreams would work
// splendid.
//
// Documentation for all of the public functions is contained in clan.h
//
//  - Alister
//
//  Aeonian Dreams:
//    telnet://dreams.game-host.org:4000
//
//********************************************************************************


#include "clan.h"        // the interface we need to impleme
#include "db.h"             // for LIB_ETC
#include "comm.h"           // for send_to_char
#include "interpreter.h"    // for ACMD()
#include "utils.h"          // for CREATE() and IDNUM()

extern char *strlwr(char *s);
extern void send_editor_help(struct descriptor_data *d);

/* Local variables */
int num_clans = 0;
struct clan_data **clan = NULL;

/* Local functions */
struct clan_member *ClanMemberFromList(const int id, struct 
clan_member *list);
int fgetlinetomax(FILE *file, char *p, int maxlen);
bool clanSave(const struct clan_data *S, const char *filename);
struct clan_data *clanLoad(const char *filename);
char *clanFilename(const struct clan_data *S);
void  clanAdd(struct clan_data *S);
void  clanRemove(struct clan_data *S);
void  clanDelete(struct clan_data *S);
void  set_clan(struct char_data *ch, char *clan);
void  remove_clan(struct char_data *ch);
struct clan_data *clanGet(const char *name);

/* Structures */
struct clan_member {

  struct clan_member *next;
  int    id;

};


struct clan_data {

  char *name;
  char *info;
  char *highrank;
  char *midrank;
  char modlist[1000];
  char memlist[1000];
  char applist[1000];

  struct clan_member *moderators;
  struct clan_member *members;
  struct clan_member *applicants;
  struct clan_rank *rank1;
  struct clan_rank *rank2;
  struct clan_rank *rank3;
  struct clan_rank *rank4;
  struct clan_rank *rank5;
  struct clan_rank *rank6;
  struct clan_rank *rank7;
  struct clan_rank *rank8;

  int  open_join;
  int  open_leave;
  long bank;
  int bany;
};

//********************************************************************************
// The implementation and documentation of local functions goes here
//********************************************************************************
//
// Return a pointer to the first member with the given ID
// return NULL if none exist.
//
struct clan_member *clanMemberFromList(const int id, struct clan_member *list) {

  for(; list != NULL; list = list->next)
    if(id == list->id)
      return list;

  return list; // NULL
}


void writeClanMasterlist() {

  int i;
  FILE *fl;
  char buf[MAX_STRING_LENGTH];

  if(!(fl = fopen(CLAN_LIST, "w"))) {
    log("ERROR: could not open clan masterlist for writing.");
    return;
  }


  sprintf(buf, "%d\n", num_clans);
  fprintf(fl, buf);
  for(i = 0; i < num_clans; i++) {
    fprintf(fl, "%s%d.cla\n", LIB_CLAN, i);
  }

  fclose(fl);
}


//
// fill up the buffer with characters until
// a newline is reached, or we hit our critical
// length. Return how many characters were read
//
int fgetlinetomax(FILE *file, char *p, const int maxlen)
{
  int count = 0;

  while(!feof(file) && count < maxlen - 1) {

    p[count] = fgetc(file);
    if(p[count] == '\n')
      break;

    count++;
  }

  p[count] = '\0';
  return count;
}


//
// return a copy of where the clan should be saved to
//
char *clanFilename(const struct clan_data *S) {

  int i;

  for(i = 0; i < num_clans; i++)
    if(!strcmp(S->name, clan[i]->name))
      break;

  if(i == num_clans)
    return NULL;
  else {
    char buf[MAX_STRING_LENGTH]; // I doubt we'll ever have a billion clans ...
    sprintf(buf, "%s%d.cla", LIB_CLAN, i);
    return strdup(buf);
  }
}


//
// Save the given clan to the given filename
// See clanSave for the format of clan files
//
struct clan_data *clanLoad(const char *filename) {


  FILE *fl;
  char line[MAX_STRING_LENGTH];
  char *info;
  int  id, infolen;
  struct clan_data *S;

  if(filename == NULL) {
    log("ERROR: passed null pointer to clanLoad");
    return NULL;
  }

  if( !(fl = fopen(filename, "r")) ) {
    log("ERROR: could not open file, %s, in clanLoad.", filename);
    return NULL;
  }


  CREATE(S, struct clan_data, 1);

  sprintf(S->modlist, "@D---@CLeaders@D---\n");
  sprintf(S->memlist, "@D---@cMembers@D---\n");
  sprintf(S->applist, "@D---@YApplicants@D---\n");
  fgetlinetomax(fl, line, MAX_STRING_LENGTH);
  sscanf(line, "%d %d", &(S->open_join), &(S->open_leave));

  fgetlinetomax(fl, line, MAX_STRING_LENGTH);
  sscanf(line, "%ld", &S->bank);

  fgetlinetomax(fl, line, MAX_STRING_LENGTH);
  sscanf(line, "%d", &S->bany);

  fgetlinetomax(fl, line, MAX_STRING_LENGTH);
  S->name = strdup(line);

  fgetlinetomax(fl, line, MAX_STRING_LENGTH);
  S->highrank = strdup(line);

  fgetlinetomax(fl, line, MAX_STRING_LENGTH);
  S->midrank = strdup(line);

  int memcount = 0;
  // load moderators
  while(TRUE) {

    struct clan_member *moderator;
    fgetlinetomax(fl, line, MAX_STRING_LENGTH);
    if(!strcmp(line, "~"))
      break;

    sscanf(line, "%d", &id);
    CREATE(moderator, struct clan_member, 1);
    moderator->id = id;
    moderator->next = S->moderators;
    S->moderators = moderator;
    if (get_name_by_id(id)) {
     memcount += 1;
     sprintf(S->modlist+strlen(S->modlist), "@D[@G%2d@D]@W %s\n", memcount, get_name_by_id(id));
    }
  }


  // load members
  while(TRUE) {

    struct clan_member *member;
    fgetlinetomax(fl, line, MAX_STRING_LENGTH);
    if(!strcmp(line, "~"))
      break;

    sscanf(line, "%d", &id);
    CREATE(member, struct clan_member, 1);
    member->id = id;
    member->next = S->members;
    S->members = member;
    if (get_name_by_id(id)) {
     memcount += 1;
     sprintf(S->memlist+strlen(S->memlist), "@D[@G%2d@D]@W %s\n", memcount, get_name_by_id(id));
    }
  }


  // load applicants
  while(TRUE) {

    struct clan_member *applicant;
    fgetlinetomax(fl, line, MAX_STRING_LENGTH);
    if(!strcmp(line, "~"))
      break;

    sscanf(line, "%d", &id);
    CREATE(applicant, struct clan_member, 1);
    applicant->id = id;
    applicant->next = S->applicants;
    S->applicants = applicant;
    if (get_name_by_id(id)) {
     sprintf(S->applist+strlen(S->applist), "@W%s\n", get_name_by_id(id));
    }
  }


  infolen = 0;
  info = strdup("");
  // keep on taking in strings until we hit a line
  // that contains a single ~
  strcpy(line, "");
  S->info = fread_string(fl, line);

  if(strlen(line) > 0) {
    // we should do something here ...
  }


  fclose(fl);
  return S;
}


//
// Save the given clan to the given filename and the format:
//
// <open_join> <open_leave>
// <clan bank>
// <clan name>
// <clan high rank name>
// <clan mid rank name>
// <moderator 1>
// <moderator 2>
// ...
// ~
// <member 1>
// <member 2>
// ...
// ~
// <applicant 1>
// <applicant 2>
// ...
// <clan info ....
//  ...>
// ~
//
bool clanSave(const struct clan_data *S, const char *filename) {

  FILE *fl;
  struct clan_member *list;

  if(filename == NULL) {
    log("ERROR: passed null pointer to clanSave when saving %s", S->name);
    return FALSE;
  }

  if( !(fl = fopen(filename, "w")) ) {
    log("ERROR: could not save clan, %s, to filename, %s.", S->name, filename);
    return FALSE;
  }

  fprintf(fl, "%d %d\n", S->open_join, S->open_leave);
  fprintf(fl, "%ld\n", S->bank);
  fprintf(fl, "%d\n", S->bany);
  fprintf(fl, "%s\n", S->name);
  fprintf(fl, "%s\n", S->highrank);
  fprintf(fl, "%s\n", S->midrank);

  for(list = S->moderators; list != NULL; list = list->next)
    fprintf(fl, "%d\n", list->id);
  fprintf(fl, "~\n");

  for(list = S->members; list != NULL; list = list->next)
    fprintf(fl, "%d\n", list->id);
  fprintf(fl, "~\n");

  for(list = S->applicants; list != NULL; list = list->next)
    fprintf(fl, "%d\n", list->id);
  fprintf(fl, "~\n");

  fprintf(fl, "%s~\n", S->info);

  fclose(fl);
  return TRUE;
}


//
// free the clan and everything it points to
//
void clanDelete(struct clan_data *S) {

  struct clan_member *next, *member;

  if(S->moderators)
    for(member = S->moderators; member != NULL; member = next) {
      next = member->next;
      free(member);
    }

  if(S->members)
    for(member = S->members; member != NULL; member = next) {
      next = member->next;
      free(member);
    }

  if(S->applicants)
    for(member = S->applicants; member != NULL; member = next) {
      next = member->next;
      free(member);
    }

  free(S->name);
  free(S->info);
  if (S->highrank)
   free(S->highrank);
  if (S->midrank)
   free(S->midrank);
  free(S);
}


//
// Remove a clan from the list of all clans,
// and then free up its memory
//
void clanRemove(struct clan_data *S) {

  int i, j;
  for(i = 0; i < num_clans; i++)
    if(clan[i] == S)
      break;


  if(i == num_clans) {
    log("ERROR: tried to remove clan, %s, which did not formally exist.", S->name);
    clanDelete(S);
    return;
  }


  num_clans--;
  for(j = i; j < num_clans; j++)
    clan[j] = clan[j + 1];
  for(; i < num_clans; i++)
    clanSave(clan[i], clanFilename(clan[i]));

  clanDelete(S);
  writeClanMasterlist();
}


//
// Add a new clan to the list of all clans
//
void clanAdd(struct clan_data *S)
{

  int i;
  struct clan_data **oldList = clan;

  /*clan = malloc( sizeof(struct clan_data *) * (num_clans) );*/
  clan = malloc( sizeof(struct clan_data *) * (num_clans + 1) );

  for(i = 0; i < num_clans; i++)
    clan[i] = oldList[i];

  clan[num_clans] = S;
  num_clans++;
  clanSave(S, clanFilename(S));
  free(oldList);
  writeClanMasterlist();
}


//
// return a pointer to the clan with the given name
// return NULL if none exist
//
struct clan_data *clanGet(const char *name) {

  int i;
  char *newname = strlwr(strdup(name));

  for(i = 0; i < num_clans; i++)
    if(!strcmp(newname, strlwr(strdup(clan[i]->name)))) {
      free(newname);
      return clan[i];
    }

  free(newname);
  return NULL;
}

//********************************************************************************
// Here are the functions we need to implement from clan.h
//********************************************************************************

bool clanReload(const char *name) {

  int i;
  struct clan_data *S;

  if(!(S = clanGet(name)))
    return FALSE;

  for(i = 0; i < num_clans; i++)
    if(S == clan[i]) {
      char buf[MAX_STRING_LENGTH];
      clanDelete(clan[i]);
      sprintf(buf, "%s%d.cla", LIB_CLAN, i);
      clan[i] = clanLoad(buf);
      return TRUE;
    }

  return FALSE;
}


void clanBoot() {

  FILE *fl;
  int i, len;
  char line[MAX_STRING_LENGTH];

  if(!(fl = fopen(CLAN_LIST, "r"))) {
    log("  Could not open clan masterlist. Aborting.");
    return;
  }

  if(feof(fl)) {
    log("  Clan masterlist contained no data! Aborting.");
    return;
  }

  len = fgetlinetomax(fl, line, MAX_STRING_LENGTH);
  sscanf(line, "%d", &num_clans);
  if(num_clans <= 0) {
    log("  No clans have formed yet.");
    clan = NULL;
    return;
  }

  clan = malloc( sizeof(struct clan_data *) * num_clans );

  for(i = 0; i < num_clans; i++) {
    if( (len = fgetlinetomax(fl, line, MAX_STRING_LENGTH)) > 0) {
      log("  Loading clan: %s", line);
      clan[i] = clanLoad(line);
    }
    else {
      log("  Found blank line while looking for clan names. Aborting.");
      for(i--; i >= 0; i--)
	clanDelete(clan[i]);
      free(clan);  // ...it would be nice, wouldn't it?
      clan = NULL;
      num_clans = 0;
      fclose(fl);
      return;
    }
  }
  fclose(fl);
}


bool isClan(const char *name) {
  return (clanGet(name) != NULL);
}


bool clanCreate(const char *name) {

  struct clan_data *S;

  if(isClan(name))
    return FALSE;

  CREATE(S, struct clan_data, 1);
  S->name = strdup(name);
  S->info = strdup(DEFAULT_CLAN_INFO);
  S->moderators = NULL;
  S->members = NULL;
  S->applicants = NULL;
  S->highrank = strdup("Captain");
  S->midrank = strdup("Lieutenant");

  clanAdd(S);
  return TRUE;
}

void clanINFOW(char *name, struct char_data *ch) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch))
   return;
  else {
  char *backstr = NULL;
  act("$n begins to edit a clan's info.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  send_editor_help(ch->desc);
  write_to_output(ch->desc, "@rYou are limited to 1000 characters for the clan info.@n\r\n");
  backstr = strdup(S->info);
  write_to_output(ch->desc, "%s\r\n", S->info);
  string_write(ch->desc, &S->info, 1000, 0, backstr);
  clanSave(S, clanFilename(S));
  }
}
void clan_update(void) {
  int i;

  if(num_clans < 1) {
    return;
  }
  for(i = 0; i < num_clans; i++) {
   clanSAFE(clan[i]->name);
  }
  return;
}

void clanSAFE(char *name) {
  struct clan_data *S = clanGet(name);

  if(S == NULL)
   return;
  else {
   clanSave(S, clanFilename(S));
  }
}

void clanDestroy(const char *name) {

  struct clan_data *S = clanGet(name);
  if(S)  clanRemove(S);
}


bool clanApply(const char *name, struct char_data *ch) {
  char buf[MAX_INPUT_LENGTH];
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch))
    return FALSE;

  if(clanMemberFromList(GET_IDNUM(ch), S->moderators) ||
     clanMemberFromList(GET_IDNUM(ch), S->members))
    return FALSE;

  if(clanMemberFromList(GET_IDNUM(ch), S->applicants))
    return TRUE;

  struct clan_member *new;
  CREATE(new, struct clan_member, 1);
  new->id = GET_IDNUM(ch);
  
  sprintf(buf, "Applying for %s", S->name);
  set_clan(ch, buf);
  new->next = S->applicants;
  S->applicants = new;
  clanSave(S, clanFilename(S));

  return TRUE;
}

bool clanHIGHRANK(const char *name, const struct char_data *ch, const char *rank) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  else {
   S->highrank = strdup(rank);
   clanSave(S, clanFilename(S));
   return TRUE;
  }
}
bool clanMIDRANK(const char *name, const struct char_data *ch, const char *rank) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  else {
   S->midrank = strdup(rank);
   clanSave(S, clanFilename(S));
   return TRUE;
  }
}

bool clanRANK(const char *name, const struct char_data *ch, struct char_data *vict, int num) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  else {
   GET_CRANK(vict) = num;
   return TRUE;
  }
}

bool clanRANKD(const char *name, struct char_data *ch, struct char_data *vict) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  else {
   send_to_char(ch, "@cClan Rank@D: @w");
   if (GET_CRANK(vict) == 0 && !clanMemberFromList(GET_IDNUM(vict), S->moderators)) {
    send_to_char(ch, "Member@n\r\n");
   }
   else if (GET_CRANK(vict) == 1 && !clanMemberFromList(GET_IDNUM(vict), S->moderators)) {
    send_to_char(ch, "%s@n\r\n", S->midrank);
   }
   else if (GET_CRANK(vict) == 2 && !clanMemberFromList(GET_IDNUM(vict), S->moderators)) {
    send_to_char(ch, "%s@n\r\n", S->highrank);
   }
   else {
    send_to_char(ch, "Leader@n\r\n");
   }
   return TRUE;  
  }
}

bool clanBANY(const char *name, const struct char_data *ch) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  if (S->bany <= 0) {
   return FALSE;
  }
  else {
   return TRUE;
  }
}

bool clanBSET(const char *name, struct char_data *ch) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  if (S->bany > 0) {
    S->bany = 0;
    send_to_char(ch, "The clan bank will now only be accessible from its room.\r\n");
    clanSave(S, clanFilename(S));
    return TRUE;
  }
  else {
    S->bany = 1;
    send_to_char(ch, "The clan bank will now be accessible from anywhere.\r\n");
    clanSave(S, clanFilename(S));
    return TRUE;
  }
}

bool clanBANKADD(const char *name, const struct char_data *ch, long amt) {

  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  else {
   S->bank += amt;
   clanSave(S, clanFilename(S));
   return TRUE;
  }
}

long clanBANK(const char *name, const struct char_data *ch) {
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  else {
   long amt = 0;
   amt = S->bank;
   return amt;
  }
}

bool clanBANKSUB(const char *name, const struct char_data *ch, long amt) {

  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch)) {
    return FALSE;
  }
  if (S->bank - amt < 0) {
    return FALSE;
  }
  else {
   S->bank -= amt;
   clanSave(S, clanFilename(S));
   return TRUE;
  }
}
bool clanInduct(const char *name, struct char_data *ch) {

  struct clan_member *m, *temp;
  struct clan_data *S = clanGet(name);
  char buf[MAX_INPUT_LENGTH];

  if(S == NULL || IS_NPC(ch))
    return FALSE;

  if(clanMemberFromList(GET_IDNUM(ch), S->moderators) ||
     clanMemberFromList(GET_IDNUM(ch), S->members))
    return TRUE;

  if( (m = clanMemberFromList(GET_IDNUM(ch), S->applicants)) ) {
    REMOVE_FROM_LIST(m, S->applicants, next, temp);
    free(m);
  }
  sprintf(buf, "%s", S->name);
  set_clan(ch, buf);
  CREATE(m, struct clan_member, 1);
  m->id = GET_IDNUM(ch);
  m->next = S->members;
  S->members = m;
  clanSave(S, clanFilename(S));
  clanReload(name);
  return TRUE;
}

void set_clan(struct char_data *ch, char *clan)
{
  if (GET_CLAN(ch) != NULL)
    free(GET_CLAN(ch));
  GET_CLAN(ch) = strdup(clan);
  GET_CRANK(ch) = 0;
}

void remove_clan(struct char_data *ch)
{
 if (GET_CLAN(ch) != NULL)
   free(GET_CLAN(ch));
 GET_CLAN(ch) = strdup("None.");
}

bool clanMakeModerator(const char *name, struct char_data *ch) 
{

  struct clan_member *m, *temp;
  struct clan_data *S = clanGet(name);
  char buf[MAX_INPUT_LENGTH];

  if(S == NULL || IS_NPC(ch))
    return FALSE;

  if(clanMemberFromList(GET_IDNUM(ch), S->moderators))
    return TRUE;

  if( (m = clanMemberFromList(GET_IDNUM(ch), S->members)) ) {
    REMOVE_FROM_LIST(m, S->members, next, temp);
    free(m);
  }
  else if( (m = clanMemberFromList(GET_IDNUM(ch), S->applicants)) ) {
    REMOVE_FROM_LIST(m, S->applicants, next, temp);
    free(m);
  }

  sprintf(buf, "%s", S->name);
  CREATE(m, struct clan_member, 1);
  set_clan(ch, buf);
  m->id = GET_IDNUM(ch);
  m->next = S->moderators;
  S->moderators = m;
  clanSave(S, clanFilename(S));
  return TRUE;
}


void clanExpel(const char *name, struct char_data *ch) {

  struct clan_member *m, *temp;
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch))
    return;
  remove_clan(ch);
  if( (m = clanMemberFromList(GET_IDNUM(ch), S->moderators)) ) {
    REMOVE_FROM_LIST(m, S->moderators, next, temp);
    free(m);
  }
  else if( (m = clanMemberFromList(GET_IDNUM(ch), S->members)) ) {
    REMOVE_FROM_LIST(m, S->members, next, temp);
    free(m);
  }
  clanSave(S, clanFilename(S));
  clanReload(name);
}


void clanDecline(const char *name, const struct char_data *ch) {

  struct clan_member *m, *temp;
  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch))
    return;

  if( (m = clanMemberFromList(GET_IDNUM(ch), S->applicants)) ) {
    REMOVE_FROM_LIST(m, S->applicants, next, temp);
    free(m);
  }
  clanSave(S, clanFilename(S));
}

void handle_clan_member_list(struct char_data *ch)
{

 if (IS_NPC(ch))
  return;

 if (!GET_CLAN(ch) || GET_CLAN(ch) == NULL)
  return;

 if (strstr(GET_CLAN(ch), "None"))
  return;

 struct clan_data *S = clanGet(GET_CLAN(ch));

 if(S == NULL)
  return;

 send_to_char(ch, S->modlist);
 send_to_char(ch, S->memlist);
 send_to_char(ch, S->applist);
 send_to_char(ch, "@n");
}

bool clanIsMember(const char *name, const struct char_data *ch) {

  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch))
    return FALSE;

  if(clanMemberFromList(GET_IDNUM(ch), S->moderators) ||
     clanMemberFromList(GET_IDNUM(ch), S->members))
    return TRUE;
  else
    return FALSE;
}


bool clanIsModerator(const char *name, const struct char_data *ch) {

  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch))
    return FALSE;

  return (clanMemberFromList(GET_IDNUM(ch), S->moderators) != NULL);
}


bool clanIsApplicant(const char *name, const struct char_data *ch) {

  struct clan_data *S = clanGet(name);

  if(S == NULL || IS_NPC(ch))
    return FALSE;

  return (clanMemberFromList(GET_IDNUM(ch), S->applicants) != NULL);
}


bool clanOpenJoin(const char *name) {

  struct clan_data *S = clanGet(name);
  return (S != NULL && S->open_join == TRUE);
}


bool clanOpenLeave(const char *name) {

  struct clan_data *S = clanGet(name);
  return (S != NULL && S->open_leave == TRUE);
}


bool clanSetOpenJoin(const char *name, const int val) {

  struct clan_data *S = clanGet(name);
  if(S == NULL)
    return FALSE;

  if(val == FALSE)    S->open_join = FALSE;
  else                S->open_join = TRUE;

  clanSave(S, clanFilename(S));
  return TRUE;
}


bool clanSetOpenLeave(const char *name, const int val) {

  struct clan_data *S = clanGet(name);
  if(S == NULL)
    return FALSE;

  if(val == FALSE)    S->open_leave = FALSE;
  else                S->open_leave = TRUE;

  clanSave(S, clanFilename(S));
  return TRUE;
}


void listClanInfo(const char *name, struct char_data *ch) {

  struct clan_data *S = clanGet(name);

  if(S == NULL) {
    send_to_char(ch, "%s is not a formal clan.\r\n", name);
    return;
  }

  send_to_char(ch, "@cClan Name        @D: @C%s\n"
	           "@cJoin Restriction @D: @C%s\n"
 	           "@cLeave Restriction@D: @C%s\n"
                   "@D---@YClan Ranks@D---@n\n"
                   "@cLeader@n\n"
                   "@c%s@n\n"
                   "@c%s@n\n"
                   "@cMember@n\n"
	           "\n"
	           "%s@n\n", 
	       S->name,
	       (S->open_join==FALSE)?"Players must be enrolled to join this clan":
	                             "Players may join this clan as they please",
	       (S->open_leave==FALSE)?"Players must be expelled to leave this clan":
	                              "Players may leave this clan as they please", S->highrank, S->midrank,
	       S->info);
}


void listClansOfVictToChar(const struct char_data *vict, struct 
char_data *ch) {

  int i;
  bool clan_found = FALSE;

  if(!IS_NPC(vict)) {
    for(i = 0; i < num_clans; i++) {

      if(clanMemberFromList(GET_IDNUM(vict), clan[i]->moderators) ||
	 clanMemberFromList(GET_IDNUM(vict), clan[i]->members)) {

	if(clan_found == FALSE) {
	  clan_found = TRUE;
	  send_to_char(ch, "Clans %s belongs to:\r\n", GET_NAME(vict));
	}
	send_to_char(ch, "  %s\r\n", clan[i]->name);
      }
    }
  }

  if(!clan_found)
    send_to_char(ch, "%s does not belong to any clans.\r\n", GET_NAME(vict));
}


void listClans(struct char_data *ch) {

  int i;

  if(num_clans < 1) {
    send_to_char(ch, "Presently, no clans have formally created.\r\n");
    return;
  }

  send_to_char(ch, "The list of clans on Dragonball Advent Truth:\r\n");
  for(i = 0; i < num_clans; i++)
    send_to_char(ch, "  %s\r\n", clan[i]->name);

}

int checkCLAN(struct char_data *ch) {
 int i;

 if(num_clans < 1) {
    return FALSE;
 }
 if (GET_CLAN(ch) == NULL) {
  return FALSE;
 }
 for(i = 0; i < num_clans; i++) {
  if (strstr(GET_CLAN(ch), clan[i]->name)) {
   return TRUE;
  }
 }
 return FALSE;
}

void checkAPP(struct char_data *ch) {
  int i;

  if(num_clans < 1) {
    return;
  }
  for(i = 0; i < num_clans; i++) {
   if (strstr(GET_CLAN(ch), clan[i]->name)) {
    GET_CLAN(ch) = strdup(clan[i]->name);
    return;
   }
  }
  return;
}
