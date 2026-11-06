#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/*
 * What the fuzz targets share. The game calls exit() on a data file it can't use, which is its way of
 * refusing the file rather than a bug, so it's linked with --wrap=exit and comes back here instead.
 */
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

extern jmp_buf fuzz_refused;
extern bool fuzz_can_refuse;

/*
 * Writes bytes to a file, with its directories. Text files only ever have what a text editor could
 * put there, which is a NUL-free file, and the game isn't expected to cope with a binary one.
 */
void fuzz_write_file(const char *path, const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif
