#pragma once

unsigned char *zmalloc(int, char *, int);
unsigned char *zrealloc(unsigned char *, int, char *, int);
void zfree2(unsigned char *, char *, int);
void zmalloc_init(void);
void zmalloc_check(void);
char *zstrdup(const char *, char *, int);

#define malloc(x)	zmalloc((x),__FILE__,__LINE__)
#define calloc(n,x)	zmalloc((n*x),__FILE__,__LINE__)
#define realloc(r,x)	zrealloc((unsigned char *)(r),(x),__FILE__,__LINE__)
#define free(x)		zfree2((unsigned char *)(x),__FILE__,__LINE__)
