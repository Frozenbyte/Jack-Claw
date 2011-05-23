#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#ifdef _WIN32
#include <windows.h>
#endif

#include <string.h>


#include "Logger.h"

#ifndef __GNUC__
// non-gcc compilers don't like attribute
#ifndef __attribute__
#define __attribute__(x)
#endif  // __attribute__
#endif  // __GNUC__


#ifdef FINAL_RELEASE_BUILD

#define unimplemented()
//#define warning(fmt, args...)

#else

#include <stdio.h>

#ifdef __GNUC__

#define unimplemented() { static bool firsttime_ = true; if (firsttime_) { unimplemented_(__PRETTY_FUNCTION__, __FILE__, __LINE__); firsttime_ = false; } }
//#define warning(fmt, args...) fprintf(stderr, fmt, ## args)

#else   // __GNUC__

#define unimplemented() { static bool firsttime_ = true; if (firsttime_) { unimplemented_("Unknown function",  __FILE__, __LINE__); firsttime_ = false; } }
//void warning(const char *fmt, ...);

#endif   // __GNUC__

#endif  //FINAL_RELEASE_BUILD


#ifdef __GLIBC__
void backtrace(void) __attribute__((no_instrument_function));
#else
static inline void backtrace(void) {};
#endif

// print stuff into a string
std::string strPrintf(const char *fmt, ...) __attribute__((format (printf, 1, 2) ));

static inline void unimplemented_(const char *function, const char *file, int line);
static inline void unimplemented_(const char *function, const char *file, int line) {
	LOG_DEBUG(strPrintf("unimplemented function %s at %s:%d", function, file, line).c_str());

#ifdef __GLIBC__
    backtrace();
#endif

}

void errorMessage(const char *msg, ...) __attribute__ ((format (printf, 1, 2))) __attribute__((no_instrument_function));

void setsighandler(void);

std::string getFontDirectory();

#define RGBI(r,g,b)          ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#define DegToRadian(degree) ((degree) * (PI / 180.0f))

enum CULLMODE {
	CULL_NONE,
	CULL_CCW,
	CULL_CW
};

#ifdef FINAL_RELEASE_BUILD

#define glErrors()

#else   // FINAL_RELEASE_BUILD

#define glErrors() checkGlErrors(__FILE__, __LINE__)

bool checkGlErrors(const char *file, int line) __attribute__((no_instrument_function));

#endif


struct NullDeleter
{
	void operator() (void *)
	{
	}
};


void sysSetArgs(int argc, char *argv[]);

// something has gone horribly wrong
// show error msg and exit
void sysFatalError(std::string msg) __attribute__((noreturn));


#if defined(_WIN64)

#define FMT_SIZE "%I64u"
#define FMT_U64 "%I64u"
#define FMT_X64 "%I64x"
#define FMT_INTPTR "%I64d"
#define FMT_UINTPTR "%I64u"

#elif defined(__x86_64)

#define FMT_SIZE "%lu"
#define FMT_U64 "%lu"
#define FMT_X64 "%lx"
#define FMT_INTPTR "%ld"
#define FMT_UINTPTR "%lu"

#elif defined(_WIN32)

#define FMT_SIZE "%u"
#define FMT_U64 "%I64u"
#define FMT_X64 "%I64x"
#define FMT_INTPTR "%d"
#define FMT_UINTPTR "%u"

#else  // assume i386

#define FMT_SIZE "%u"
#define FMT_U64 "%llu"
#define FMT_X64 "%llx"
#define FMT_INTPTR "%d"
#define FMT_UINTPTR "%u"

#endif  // machine-specific printfs


#endif  // MISCELLANEOUS_H
