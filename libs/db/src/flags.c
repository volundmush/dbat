#include "dbat/db/flags.h"
#include <stdio.h>
#include <string.h>

int flag_test(const bitvector_t bitvector[], int flag)
{
  int array_pos = flag / 32;
  int bit_pos = flag % 32;

  return (bitvector[array_pos] & (1u << bit_pos)) != 0;
}

void flag_set(bitvector_t bitvector[], int flag, int value)
{
  int array_pos = flag / 32;
  int bit_pos = flag % 32;

  if (value) {
    bitvector[array_pos] |= (1 << bit_pos);
  } else {
    bitvector[array_pos] &= ~(1 << bit_pos);
  }
}

int flag_toggle(bitvector_t bitvector[], int flag)
{
  int array_pos = flag / 32;
  int bit_pos = flag % 32;

  bitvector[array_pos] ^= (1 << bit_pos);
  return (bitvector[array_pos] & (1 << bit_pos)) != 0;
}

size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  if (reslen == 0) {
    return 0;
  }

  size_t len = 0;
  int nr = 0;

  while (bitvector && len < reslen) {
    if (bitvector & 1) {
      const char *name = (*names[nr] != '\n') ? names[nr] : "UNDEFINED";
      int nlen = snprintf(result + len, reslen - len, "%s ", name);
      if (nlen < 0 || (size_t)nlen >= reslen - len) {
        result[len] = '\0';
        return len;
      }
      len += (size_t)nlen;
    }

    if (*names[nr] != '\n') {
      nr++;
    }

    bitvector >>= 1;
  }

  if (len == 0) {
    strlcpy(result, "None ", reslen);
    len = 4;
  } else {
    result[len - 1] = '\0'; // Remove trailing space
    len--;
  }

  return len;
}

size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type > 0 && *names[nr] != '\n') {
    type--;
    nr++;
  }

  const char *name = (*names[nr] != '\n') ? names[nr] : "UNDEFINED";
  return strlcpy(result, name, reslen);
}

size_t sprintbitarray(bitvector_t bitvector[], const char *names[], int maxar, char *result, size_t reslen)
{
  if (reslen == 0) {
    return 0;
  }

  size_t len = 0;
  int found_end = FALSE;

  for (int teller = 0; teller < maxar && !found_end; teller++) {
    for (int nr = 0; nr < 32 && !found_end; nr++) {
      int flag = (teller * 32) + nr;
      const char *name = names[flag];

      if (*name == '\n') {
        found_end = TRUE;
        break;
      }

      if (flag_test(bitvector, flag)) {
        if (*name != '\0') {
          if (len < reslen) {
            int nlen = snprintf(result + len, reslen - len, "%s ", name);
            if (nlen > 0) {
              len += (size_t)nlen;
            }
          }
        }
      }
    }
  }

  if (len == 0) {
    strlcpy(result, "None ", reslen);
    len = 4;
  } else if (len > 0) {
    result[len - 1] = '\0';
    len--;
  }

  return len;
}

int get_flag_by_name(const char *flag_list[], char *flag_name) 
{ 
   int i=0; 
   for (;flag_list[i] && *flag_list[i] && strcmp(flag_list[i], "\n") != 0; i++) 
     if (!strcmp(flag_list[i], flag_name)) 
       return (i); 
   return (NOFLAG); 
}

size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}