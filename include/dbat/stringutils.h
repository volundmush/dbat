#pragma once
#include <cstring>

#ifdef _WIN32
extern int strcasecmp(const char* s1, const char* s2);
extern int strncasecmp(const char* s1, const char* s2, size_t n);
#else
#include <strings.h>
#endif
extern size_t strlcpy(char *dst, const char *src, size_t dsize);