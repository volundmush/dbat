#pragma once


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