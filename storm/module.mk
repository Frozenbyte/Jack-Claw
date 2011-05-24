sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


# process subdirectories
SUBDIRS:= \
		 storm3dv2 \
		 stormgl \
		 # empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


# files in this directory
FILES:= \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
