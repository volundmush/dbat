#include "dbat/game/stringutils.h"
#include "dbat/db/utils.h"
#include "dbat/game/log.h"

int levenshtein_distance(char *s1, char *s2)
{
  int s1_len = strlen(s1), s2_len = strlen(s2);
  int **d, i, j;

  CREATE(d, int *, s1_len + 1);
  for (i = 0; i <= s1_len; i++) {
    CREATE(d[i], int, s2_len + 1);
    d[i][0] = i;
  }

  for (j = 0; j <= s2_len; j++)
    d[0][j] = j;
  for (i = 1; i <= s1_len; i++)
    for (j = 1; j <= s2_len; j++)
      d[i][j] = MIN(d[i - 1][j] + 1, MIN(d[i][j - 1] + 1,
      d[i - 1][j - 1] + ((s1[i - 1] == s2[j - 1]) ? 0 : 1)));

  i = d[s1_len][s2_len];

  for (j = 0; j <= s1_len; j++)
    free(d[j]);
  free(d);

  return i;
}

int count_color_chars(char *string)
{
  int i, len;
  int num = 0;

        if (!string || !*string)
                return 0;

        len = strlen(string);
  for (i = 0; i < len; i++) {
    while (string[i] == '@') {
      if (string[i + 1] == '@') {
        num++;
      } else if (string[i + 1] == '[') {
        num += 4;
      } else {
        num += 2;
      }
      i += 2;
    }
  }
  return num;
}

/* Trims leading and trailing spaces from string */
void trim(char *s)
{
	// Trim spaces and tabs from beginning:
	int i=0,j;
	while((s[i]==' ')||(s[i]=='\t')) {
		i++;
	}
	if(i>0) {
		for(j=0;j<strlen(s);j++) {
			s[j]=s[j+i];
		}
	s[j]='\0';
	}

	// Trim spaces and tabs from end:
	i=strlen(s)-1;
	while((s[i]==' ')||(s[i]=='\t')) {
		i--;
	}
	if(i<(strlen(s)-1)) {
		s[i+1]='\0';
	}
}


/* Turns number into string and adds commas to it. */
char *add_commas(int64_t num)
{ 
  #define DIGITS_PER_GROUP      3 
  #define BUFFER_COUNT         19 
  #define DIGITS_PER_BUFFER    25 

  int64_t i, j, len, negative = (num < 0);
  char num_string[DIGITS_PER_BUFFER]; 
  static char comma_string[BUFFER_COUNT][DIGITS_PER_BUFFER]; 
  static int64_t which = 0;

  sprintf(num_string, "%" I64T "", num);
  len = strlen(num_string); 

  for (i = j = 0; num_string[i]; ++i) { 
    if ((len - i) % DIGITS_PER_GROUP == 0 && i && i - negative) 
      comma_string[which][j++] = ','; 
    comma_string[which][j++] = num_string[i]; 
  } 
  comma_string[which][j] = '\0'; 

  i = which; 
  which = (which + 1) % BUFFER_COUNT; 

  return comma_string[i]; 

  #undef DIGITS_PER_GROUP 
  #undef BUFFER_COUNT 
  #undef DIGITS_PER_BUFFER 
}

char *CAP(char *txt)
{
  int i;
  for (i = 0; txt[i] != '\0' && (txt[i] == '@' && IS_COLOR_CHAR(txt[i + 1])); i += 2);

  txt[i] = UPPER(txt[i]);
  return (txt);
}

char *strlwr(char *s) 
{ 
   if (s != NULL) 
   { 
      char *p; 

      for (p = s; *p; ++p) 
         *p = LOWER(*p); 
   } 
   return s; 
}

/* Strips \r\n from end of string.  */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}

char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/*
 * one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word.
 *
 * No longer ignores fill words.  -dak, 6 Jan 2003.
 */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

/*
 * Same as two_arguments only, well you get the idea... - Iovan
 *
 */
char *three_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg)
{
 return (one_argument(one_argument(one_argument(argument, first_arg), second_arg), third_arg)); /* >.> */
}

int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}

/*
 * Return first space-delimited token in arg1; remainder of string in arg2.
 *
 * NOTE: Requires sizeof(arg2) >= sizeof(string)
 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  if (arg2 != temp)
  strcpy(arg2, temp);	/* strcpy: OK (documentation) */
}

void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}

/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;


  while (*ddread)   /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
	ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae.
   */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}

int is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}

char *fname(const char *namelist)
{
  static char holder[READ_SIZE];
  char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}