
#include "precompiled.h"

#undef NDEBUG

#pragma warning(disable:4103)
#pragma warning(disable:4786)

#include "assert.h"
#include "../system/Logger.h"

#include <boost/lexical_cast.hpp>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif  // _WIN32

using namespace std;
using namespace boost;

namespace frozenbyte {
namespace {

	void removeMessages()
	{
#ifdef _WIN32
		MSG msg = { 0 };
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_PAINT)
				return;
		}
#endif  // _WIN32
	}

} // unnamed

void assertImp(const char *predicateString, const char *file, int line)
{
	string error = predicateString;
	error += " (";
	error += file;
	error += ", ";
	error += lexical_cast<string> (line);
	error += ")";

	Logger::getInstance()->error(error.c_str());
	removeMessages();

	// can't use the actual assert clause here, because it 
	// has been lost in the function call (a macro assert would be 
	// the only possible solution, but that would cause problems
	// with the #undef NDEBUG in the header...)
#ifdef FB_TESTBUILD
	assert(!"testbuild fb_assert encountered.");
#endif
}

} // frozenbyte
