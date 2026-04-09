#pragma once


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


/* memory utils **********************************************************/





/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'cmtemp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next, cmtemp)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      cmtemp = head;			\
      while (cmtemp && (cmtemp->next != (item))) \
	 cmtemp = cmtemp->next;		\
      if (cmtemp)				\
         cmtemp->next = (item)->next;	\
   }					\

#define REMOVE_FROM_DOUBLE_LIST(item, head, next, prev)\
      if((item) == (head))			\
      {						\
            head = (item)->next;  		\
            if(head) head->prev = NULL;		\
      }						\
      else					\
      {						\
        temp = head;				\
          while(temp && (temp->next != (item)))	\
            temp = temp->next;			\
             if(temp)				\
            {					\
               temp->next = item->next;		\
               if(item->next)			\
                item->next->prev = temp;	\
            }					\
      }						\

