
#ifndef INT64_TO_HEX_H
#define INT64_TO_HEX_H


#ifdef _MSC_VER
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else  // _MSC_VER
#include <stdint.h>

#endif  // _MSC_VER


extern char *int64_to_hex(int64_t value);
extern int64_t hex_to_int64(const char *string);
extern int hex_to_int64_errno();

#endif
