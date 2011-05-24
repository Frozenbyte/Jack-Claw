#include "precompiled.h"

// Copyright 2002-2004 Frozenbyte Ltd.

#pragma warning(disable:4103)
#pragma warning(disable:4786)

#include <string.h>
#include <assert.h>
#include "int64_to_hex.h"
#include "../util/Debug_MemoryManager.h"


char hex_convert_strbuf[16+1];

int _last_hex_to_int64_errno = 0;

char *int64_to_hex(int64_t value_orig)
{
	uint64_t value = value_orig;

  uint64_t tmp;
  uint64_t expv;
  int strpos = 0;

	// note, this is a really silly way of doing this. should rather use a shifting bitmask.
  for (expv = 0x1000000000000000LL; expv > 0; expv /= 16)
  {
    tmp = (value / expv);
    value -= tmp * expv;
		assert(tmp >= 0 && tmp < 16);
		if (tmp >= 10)
			hex_convert_strbuf[strpos] = (char)('A' + (tmp - 10));
		else
			hex_convert_strbuf[strpos] = (char)('0' + tmp);
    strpos++;
  }

	assert(strpos == 16);
  hex_convert_strbuf[strpos] = '\0';

  return hex_convert_strbuf;
}



// does not check for possible overflow...

int64_t hex_to_int64(const char *string)
{
  int i;
  int len = strlen(string);
  uint64_t value = 0;
  uint64_t exp = 1;

	assert(len == 16);
  if (len != 16) 
	{
    _last_hex_to_int64_errno = 1;
		return 0;
	}

  for (i = len - 1; i >= 0; i--)
  {
    if (string[i] >= '0' && string[i] <= '9')
    {
      value += (string[i] - '0') * exp;
      exp *= 16;
		}
    else if (string[i] >= 'A' && string[i] <= 'F')
    {
      value += (string[i] - 'A' + 10) * exp;
      exp *= 16;
		}
    else if (string[i] >= 'a' && string[i] <= 'f')
    {
      value += (string[i] - 'a' + 10) * exp;
      exp *= 16;
    } else {
      _last_hex_to_int64_errno = 1;
      return 0;
    }
  }
  _last_hex_to_int64_errno = 0;
  return value;
}



int hex_to_int64_errno()
{
  return _last_hex_to_int64_errno;
}


