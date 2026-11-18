#pragma once
#include <ctime>
#include <string>

#include "dbat/consts/types.hpp"

/* ****** MUD MAIL SYSTEM HEADER FILE **********************
 ***     written by Jeremy Elson (jelson@circlemud.org) ***
 ********************************************************* */

/* INSTALLATION INSTRUCTIONS in MAIL.C */

/* You can modify the following constants to fit your own MUD.  */

/* minimum level a player must be to send mail	*/
constexpr int MIN_MAIL_LEVEL = 3;

/* # of gold coins required to send mail	*/
constexpr int STAMP_PRICE = 10;

/* Maximum size of mail in bytes (arbitrary)	*/
constexpr int MAX_MAIL_SIZE = 4096;

/* A letter waiting to be received */
struct mail_letter {
  long to{};      /* idnum of its recipient */
  long from{};    /* idnum of its sender, or -1337 for the auctioneer */
  time_t time{};  /* when it was sent */
  std::string text{};
};

/* Reads etc/plrmail at boot, converting the old binary file if that is what it is; false if it can't be read */
bool scan_file(GameWorld *g);
/* Writes every letter to etc/plrmail; with backup, what it held before is kept as etc/plrmail.bak */
bool save_mail_file(GameWorld *g, bool backup = false);
bool has_mail(GameWorld *g, long recipient);
void store_mail(GameWorld *g, long to, long from, char *message_pointer);

void notify_if_playing(GameWorld *g, struct char_data *from, int recipient_id);

SPECIAL(postmaster);

