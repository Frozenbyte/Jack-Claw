sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


# process subdirectories
SUBDIRS:= \
		 # empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


# files in this directory
FILES:= \
		ParticlePhysics.cpp \
		cloudparticlesystem.cpp \
		crappyfont.cpp \
		floattrack.cpp \
		modelparticlesystem.cpp \
		parseutil.cpp \
		particleeffect.cpp \
		particleforces.cpp \
		particlesystem.cpp \
		particlesystemmanager.cpp \
		pointarrayparticlesystem.cpp \
		sprayparticlesystem.cpp \
		vectortrack.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
