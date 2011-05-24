
#ifndef UNIQUEEDITOROBJECTHANDLE_H
#define UNIQUEEDITOROBJECTHANDLE_H

#ifdef _MSC_VER
typedef __int64 UniqueEditorObjectHandle;

#else
#include <stdint.h>

typedef int64_t UniqueEditorObjectHandle;

#endif  // _WIN32

// (constructed as follows - but do NOT rely on this, may change at any time!)
// bits 0-31 ... (unsigned int32) the internal handle given as parameter when created
// bits 32-51 ... (unsigned int20) the ue value iterator (saved on disk at exit/restored at startup)
// bits 52-62 ... ("unsigned int11") the "hash code?" for the computer specific unique string 
// bits 63 ... (signed bit) reserved

#endif

