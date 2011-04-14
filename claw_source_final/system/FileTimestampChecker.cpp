// Copyright 2002-2004 Frozenbyte Ltd.

#include "precompiled.h"

#pragma warning(disable:4103)
#pragma warning(disable:4786)

#include "FileTimestampChecker.h"
#include "../filesystem/input_stream_wrapper.h"

#ifndef _MSC_VER
#error FileTimestampChecker implemented for msvc only.
#endif

#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace frozenbyte;
#include "../util/Debug_MemoryManager.h"

bool FileTimestampChecker::isFileNewerThanFile(const char *file, const char *thanfile)
{
	// commented out so no-one will accidentally use this.
	// see the ...NewerOrSame... - that's what you probably want
	assert(!"isFileNewerThanFile - is this really what you want?");

  if (getFileTimestamp(file) > getFileTimestamp(thanfile))
    return true;
  else
    return false;
}


bool FileTimestampChecker::isFileNewerOrSameThanFile(const char *file, const char *thanfile)
{
  if (getFileTimestamp(file) >= getFileTimestamp(thanfile))
    return true;
  else
    return false;
}


bool FileTimestampChecker::isFileNewerOrAlmostSameThanFile(const char *file, const char *thanfile)
{
	// WARNING: max 60 seconds older file is also accepted as newer!
  if (getFileTimestamp(file) + 60 >= getFileTimestamp(thanfile))
    return true;
  else
    return false;
}

bool FileTimestampChecker::isFileUpToDateComparedTo(const char *file, const char *thanfile)
{
	FILE *f = fopen(file, "rb");
	if(f)
	{
		fclose(f);
		return FileTimestampChecker::isFileNewerOrAlmostSameThanFile(file, thanfile);
	}

	filesystem::FB_FILE *fp = filesystem::fb_fopen(file, "rb");
	if(fp)
	{
		filesystem::fb_fclose(fp);
		return true;
	}

	return false;
}

int FileTimestampChecker::getFileTimestamp(const char *file)
{  
  struct _stat buf;
  FILE *f;
  int fh, result;
  int ret;

  f = fopen(file, "rb");
  if (f == NULL) return -1;

  fh = _fileno(f);

  result = _fstat(fh, &buf);

  if (result != 0)
  {
    return -1;
  } else {
    ret = int(buf.st_mtime);
  }

  fclose(f);

  return ret;
}

