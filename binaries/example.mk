# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


# compiler options etc
CXX:=g++

# link time optimization, gcc >4.6 required
LTO:=n


DIRECT3D:=n
OPENGL:=y
LIBAVCODEC:=y
# null fmod openal
SOUND:=openal


# optimization options
OPTFLAGS:=-Os -ffast-math
OPTFLAGS+=-fno-strict-aliasing


CXXFLAGS:=-g
CXXFLAGS+=-I$(TOPDIR) -I$(TOPDIR)/claw_proto -I$(TOPDIR)/storm/include -I$(TOPDIR)/storm/keyb3
CXXFLAGS+=$(shell sdl-config --cflags)
CXXFLAGS+=-include $(TOPDIR)/claw_proto/configuration.h
CXXFLAGS+=-DLIGHT_MAX_AMOUNT=5
#CXXFLAGS+=-D_DEBUG
CXXFLAGS+=-DNDEBUG
# in a perfect world...
#CXXFLAGS:=-Wall -Wextra -Werror
# in the real world...
CXXFLAGS+=-w


# linker settings
LDFLAGS:=-g -Wl,-rpath,\$$$$ORIGIN/lib32
LDLIBS:=$(shell sdl-config --libs)
LDLIBS+=-lGLEW -lGL -lGLU
LDLIBS+=-lSDL_image -lSDL_ttf -lSDL_sound
#LDLIBS+=-L/usr/local/lib -lfmod-3.75
LDLIBS+=-lminizip -lz
LDLIBS+=-lpthread

LDLIBS_fmod:=-lfmod
LDLIBS_openal:=-lopenal -lvorbisfile -lboost_thread
LDLIBS_avcodec:=-lavcodec -lavformat -lswscale -lavutil

OBJSUFFIX:=.o
EXESUFFIX:=-bin


# NX32 or NX64 depending on your OS
# PhysX apparently can't pick for itself
CXXFLAGS+=-DNX32
CXXFLAGS+=-DLINUX


PHYSX_VERSION:=2.8.3
PHYSX_LIBDIR:=/usr/lib/PhysX
PHYSX_INCDIR:=/usr/include/PhysX

PHYSX_PARTS:=Physics Cooking NxExtensions NxCharacter Foundation PhysXLoader

PHYSX_INCLUDE_DIRS:=$(foreach dir,$(PHYSX_PARTS), $(PHYSX_INCDIR)/v$(PHYSX_VERSION)/SDKs/$(dir)/include)
LPHYSX:=-L$(PHYSX_LIBDIR)/v$(PHYSX_VERSION) -lPhysXLoader -ldl

# physx fluids on linux are broken
# fuck ageia and nvidia
# only on old version of PhysX
ifeq ($(PHYSX_VERSION),2.8.1)

CXXFLAGS+=-DNX_DISABLE_FLUIDS

endif

IPHYSX:=$(foreach dir,$(PHYSX_INCLUDE_DIRS),-I$(dir))
