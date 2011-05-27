# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


# compiler options etc
CXX:=i586-mingw32msvc-g++

# link time optimization, gcc >4.6 required
LTO:=n


DIRECT3D:=y
OPENGL:=y
# null fmod openal
SOUND:=fmod

# optimization options
OPTFLAGS:=-O -ffast-math
OPTFLAGS+=-fno-strict-aliasing


CXXFLAGS:=-g
CXXFLAGS+=-I$(TOPDIR) -I$(TOPDIR)/claw_proto -I$(TOPDIR)/storm/include -I$(TOPDIR)/storm/keyb3
CXXFLAGS+=-include $(TOPDIR)/claw_proto/configuration.h
CXXFLAGS+=-DLIGHT_MAX_AMOUNT=5
CXXFLAGS+=$(shell i386-mingw32msvc-sdl-config --cflags)
# in a perfect world...
#CXXFLAGS:=-Wall -Wextra -Werror
# in the real world...
CXXFLAGS+=-w


# linker settings
LDFLAGS:=-g
LDLIBS:=-lwinmm -lcomdlg32 -lgdi32
LDLIBS+=PhysXLoader.lib
LDLIBS+=$(TOPDIR)/storm/lib/minizip.lib
LDLIBS+=$(TOPDIR)/storm/lib/zlib.lib
LDLIBS_d3d:=-ld3d9 -ld3dx9_33 -ldinput8 -ldxguid -lvfw32
LDLIBS_opengl:=glew32.dll -lopengl32 -lglu32 -lSDL SDL_image.lib SDL_ttf.lib

LDLIBS_fmod:=-lfmod
LDLIBS_openal:=-lboost-thread


OBJSUFFIX:=.o
EXESUFFIX:=.exe


PHYSX_VERSION:=2.8.3
PHYSX_LIBDIR:=/usr/lib/PhysX
PHYSX_INCDIR:=/usr/include/PhysX

PHYSX_PARTS:=Physics Cooking NxExtensions NxCharacter Foundation PhysXLoader

PHYSX_INCLUDE_DIRS:=$(foreach dir,$(PHYSX_PARTS), $(PHYSX_INCDIR)/v$(PHYSX_VERSION)/SDKs/$(dir)/include)
LPHYSX:=-L$(PHYSX_LIBDIR)/v$(PHYSX_VERSION)

IPHYSX:=$(foreach dir,$(PHYSX_INCLUDE_DIRS),-I$(dir))
