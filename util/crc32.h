#ifndef UTIL_CRC32
#define UTIL_CRC32

#include "../system/Miscellaneous.h"

unsigned int crc32_file(const char *filename) __attribute__((visibility("default")));

#endif
