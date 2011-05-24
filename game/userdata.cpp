#include "precompiled.h"
#include "userdata.h"

#ifndef _WIN32
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "system/Miscellaneous.h"

#endif  // _WIN32


// User data directory handling

void createDirectoryInternal(const std::string &path)
{
#ifndef _WIN32
  static struct stat st;
  if (stat(path.c_str(),&st) == 0) {
    if (S_ISDIR(st.st_mode) && st.st_mode&S_IWUSR && st.st_mode&S_IRUSR && st.st_mode&S_IXUSR)
      return;
  } else if (mkdir(path.c_str(),0777) == 0) return;
  LOG_WARNING(strPrintf("Unable to access or create directory: %s.\n",path.c_str()).c_str());
  exit(-1);
#endif
}

// Returns the path to the user data directory.
// The directory will be created if it doesnt exist yet.
const std::string &initializeUserDataInternal()
{
	static bool initialized = false;
	static std::string result = "";
	if (!initialized) {
		std::string publisherDir = "";

#ifndef WIN32
		if (char *home = getenv("HOME")) {
#if defined(PROJECT_SHADOWGROUNDS)
#ifdef __APPLE__
			publisherDir = std::string(home) + "/Library";
			result = publisherDir + "/Shadowgrounds/";
#else
			publisherDir = std::string(home) + "/.frozenbyte";
			result = publisherDir + "/shadowgrounds/";
#endif
#elif defined(PROJECT_SURVIVOR)
#ifdef __APPLE__
			publisherDir = std::string(home) + "/Library";
			result = publisherDir + "/Survivor/";
#else
			publisherDir = std::string(home) + "/.frozenbyte";
			result = publisherDir + "/survivor/";
#endif

#elif defined(PROJECT_CLAW_PROTO)
#ifdef __APPLE__
			publisherDir = std::string(home) + "/Library";
			result = publisherDir + "/Claw/";
#else
			publisherDir = std::string(home) + "/.frozenbyte";
			result = publisherDir + "/claw/";
#endif

#else // #if defined(PROJECT_SHADOWGROUNDS)

#error "No project defined at compile time."
			LOG_WARNING("No project defined at compile time.\n");
			backtrace();
			exit(-1);
#endif // #if defined(PROJECT_SHADOWGROUNDS)

		} else {
			LOG_WARNING("No HOME environment variable set. User files will be placed into the current directory.\n");
			result = "";
		}
		if (publisherDir != "")
			createDirectoryInternal(publisherDir);
		createDirectoryInternal(result);
		createDirectoryInternal(result+"profiles");
		createDirectoryInternal(result+"config");
		createDirectoryInternal(result+"screenshots");
#endif // #ifndef WIN32
		initialized = true;
  }
  return result;
}

std::string getUserDataPrefix()
{
  return initializeUserDataInternal();
}

std::string mapUserDataPrefix(const std::string &path)
{
  if (path != "")
    return initializeUserDataInternal() + unmapUserDataPrefix(path);
  else
    return "";
}

std::string unmapUserDataPrefix(const std::string &path)
{
  static const std::string prefix = initializeUserDataInternal();
  if (path.substr(0,prefix.length()) == prefix)
    return path.substr(prefix.length());
  else
    return path;
}

void initializeUserData()
{
  initializeUserDataInternal();
}
