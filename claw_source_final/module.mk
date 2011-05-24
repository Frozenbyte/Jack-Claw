.PHONY: default all bindirs clean distclean


.SUFFIXES:

#initialize these
PROGRAMS:=
# directories which might contain object files
# used for both clean and bindirs
ALLDIRS:=

default: all


# if using LTO, add optimization flags to linker parameters
ifeq ($(LTO),y)
OPTFLAGS+=-flto
LDFLAGS+=$(OPTFLAGS) -fwhole-program
CXXFLAGS+=$(OPTFLAGS)

else

CXXFLAGS+=$(OPTFLAGS)

endif


ifeq ($(LIBAVCODEC),y)
CXXFLAGS+=-DUSE_LIBAVCODEC
LDLIBS+=$(LDLIBS_avcodec)

endif


# physics thing
CXXFLAGS+=$(IPHYSX)
LDLIBS+=$(LPHYSX)


# sound libraries
LDLIBS+=$(LDLIBS_$(SOUND))


# (call directory-module, dirname)
define directory-module

# save old
DIRS_$1:=$$(DIRS)

dir:=$1
include $(TOPDIR)/$1/module.mk

# restore saved
DIRS:=$$(DIRS_$1)

endef


# recurse directories
DIRS:=claw_proto container convert editor filesystem game mingw net ogui particle_editor2 physics sound storm system ui util
$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


TARGETS:=$(foreach PROG,$(PROGRAMS),$(PROG)$(EXESUFFIX))

all: $(TARGETS)



# create directories which might contain object files
bindirs:
	mkdir -p $(ALLDIRS)


clean:
	rm -f $(TARGETS) $(foreach dir,$(ALLDIRS),$(dir)/*$(OBJSUFFIX))

distclean: clean
	rm -f $(foreach dir,$(ALLDIRS),$(dir)/*.d)


%$(OBJSUFFIX): %.cpp
	$(CXX) -c -MT$@ -MF$*.d -MP -MMD $(CXXFLAGS) -I. -o$@ $<


# $(call program-target, progname)
define program-target

$1_SRC+=$$(foreach module, $$($1_MODULES), $$(SRC_$$(module)))
$1_OBJ:=$$($1_SRC:.cpp=$(OBJSUFFIX))
$1$(EXESUFFIX): $$($1_OBJ)
	$(CXX) $(LDFLAGS) -o$$@ $$^ $(LDLIBS) $$(foreach module, $$($1_MODULES), $$(LDLIBS_$$(module))) $$($1_LIBS)

endef


$(eval $(foreach PROGRAM,$(PROGRAMS), $(call program-target,$(PROGRAM)) ) )
