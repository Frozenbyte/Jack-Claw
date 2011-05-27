# example of local configuration
# copy to local.mk


# this is for building with msvgcc
# The glorious and horrible Visual-C-on-Linux hack
# http://github.com/turol/msvgcc


# location of source
TOPDIR:=..


# compiler options etc
CXX:=./msvgcc

# link time optimization, gcc >4.6 required
LTO:=n


DIRECT3D:=y
OPENGL:=y
# null fmod openal
SOUND:=fmod

# default to debugging enabled
DEBUG:=y


# optimization options
OPTFLAGS:=


# unlike gcc, msvc can't do debugging with optimization on
ifeq ($(DEBUG),n)

# /O1 minimize space
# /O2 maximize speed
# /Oi enable intrinsic functions
# /GA optimize for Windows Application
# /GF enable read-only string pooling
# /fp:fast "fast" floating-point model; results are less predictable
# /arch:<SSE|SSE2>
OPTFLAGS+=/O1 /Oi /GA /GF /fp:fast /arch:SSE2

endif


CXXFLAGS:=
CXXFLAGS+=-I$(TOPDIR) -I$(TOPDIR)/claw_proto -I$(TOPDIR)/storm/include -I$(TOPDIR)/storm/keyb3
CXXFLAGS+=/FI$(TOPDIR)/claw_proto/configuration.h
CXXFLAGS+=-DLIGHT_MAX_AMOUNT=5
CXXFLAGS+=-DDISABLE_STORM_DLL

# enable exceptions or the compiler complains about its standard headers
CXXFLAGS+=/EHsc

CXXFLAGS+=-D_CRT_SECURE_NO_DEPRECATE


# linker settings
LDFLAGS:=/subsystem:windows /nodefaultlib:libcmt.lib
LDLIBS:=winmm.lib comdlg32.lib gdi32.lib user32.lib
LDLIBS+=PhysXLoader.lib
LDLIBS+=minizip.lib zlib.lib
LDLIBS_d3d:=d3d9.lib d3dx9.lib dinput8.lib dxguid.lib vfw32.lib
LDLIBS_opengl:=glew32.lib opengl32.lib glu32.lib SDL.lib SDL_image.lib SDL_ttf.lib

LDLIBS_fmod:=fmodvc.lib
LDLIBS_openal:=boost-thread.lib


# FIXME: WTF is this?
LDLIBS+=/nodefaultlib:LIBCD.lib


OBJSUFFIX:=.obj
EXESUFFIX:=.exe


PHYSX_VERSION:=2.8.3
PHYSX_LIBDIR:=/usr/lib/PhysX
PHYSX_INCDIR:=/usr/include/PhysX

PHYSX_PARTS:=Physics Cooking NxExtensions NxCharacter Foundation PhysXLoader

PHYSX_INCLUDE_DIRS:=$(foreach dir,$(PHYSX_PARTS), $(PHYSX_INCDIR)/v$(PHYSX_VERSION)/SDKs/$(dir)/include)
LPHYSX:=

IPHYSX:=$(foreach dir,$(PHYSX_INCLUDE_DIRS),-I$(dir))


# different libs required for debugging...
ifeq ($(DEBUG),y)

CXXFLAGS+=/MTd -D_DEBUG
# /RTC1 Enable fast checks (/RTCsu)
# /Z7 enable old-style debug info
CXXFLAGS+=/RTC1 /Z7
LDFLAGS+=/nodefaultlib:msvcrt.lib

else

CXXFLAGS+=/MT -DNDEBUG
LDLIBS+=libcmt.lib
LDFLAGS+=/nodefaultlib:msvcrt.lib

endif
