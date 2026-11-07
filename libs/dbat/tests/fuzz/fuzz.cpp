#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>

#include "fuzz.hpp"

jmp_buf fuzz_refused;
bool fuzz_can_refuse = false;

extern "C" [[noreturn]] void __real_exit(int status);
extern "C" [[noreturn]] void __wrap_exit(int status);

void __wrap_exit(int status)
{
  if (fuzz_can_refuse)
    longjmp(fuzz_refused, 1);
  __real_exit(status);
}

void fuzz_write_file(const char *path, const uint8_t *data, size_t size)
{
  char directory[512];
  char *slash;
  FILE *fl;

  snprintf(directory, sizeof(directory), "%s", path);
  for (slash = strchr(directory, '/'); slash; slash = strchr(slash + 1, '/')) {
    *slash = '\0';
    mkdir(directory, 0755);
    *slash = '/';
  }
  if (!(fl = fopen(path, "w")))
    return;
  for (size_t i = 0; i < size; i++)
    if (data[i]) /* see fuzz.hpp */
      putc(data[i], fl);
  fclose(fl);
}
