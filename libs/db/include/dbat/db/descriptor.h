#pragma once
#include <zlib.h>
#include "consts/types.h"
#include "consts/maximums.h"
#include "consts/constates.h"


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};

struct compr {
    int state; /* 0 - off. 1 - waiting for response. 2 - compress2 on */

    Bytef *buff_out;
    int total_out; /* size of input buffer */
    int size_out; /* size of data in output buffer */

    Bytef *buff_in;
    int total_in; /* size of input buffer */
    int size_in; /* size of data in input buffer */

    z_streamp stream;
};

struct descriptor_data {
   socklen_t	descriptor;	/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   int8_t	bad_pws;	/* number of bad pw attemps this login	*/
   int8_t idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;		/* backup string for modify-str system	*/
   size_t max_str;	        /* maximum size of string in modify-str	*/
   int32_t mail_to;		/* name for mail system			*/
   int	has_prompt;		/* is the user at a prompt?             */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct oasis_olc_data *olc;   /* OLC info                            */
   struct compr *comp;                /* compression info */
   char *user;                   /* What user am I?                     */
   char *email;                  /* User Account Email.                 */
   char *pass;                   /* User Account Password.              */
   char *loadplay;               /* What character am I loading?        */
   int writenew;                 /* What slot am I writing to?          */
   int total;                    /* What Is My Total Character Limit?   */
   int rpp;                      /* What is my total RPP?               */
   char *tmp1;
   char *tmp2;
   char *tmp3;
   char *tmp4;
   char *tmp5;
   int level;
   char *newsbuf;
   /*---------------Player Level Object Editing Variables-------------------*/
   int obj_editval;
   int obj_editflag;
   char *obj_was;
   char *obj_name;
   char *obj_short;
   char *obj_long;
   int obj_type;
   int obj_weapon;
   struct obj_data *obj_point;
   /*---------------Ship Construction Editing Variables---------------------*/
   int shipmenu;
   int shipsize;
   char *ship_name;
   int shipextra[4];
   int shields;
   int armor;
   int drive;
   int shipweap;
   /*-----------------------------------------------------------------------*/
   int user_freed;
   int customfile;
   char *title;
   int rbank;
};


extern struct descriptor_data *descriptor_list;