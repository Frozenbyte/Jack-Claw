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
		AbstractPhysicsObject.cpp \
		BoxPhysicsObject.cpp \
		CapsulePhysicsObject.cpp \
		CarPhysicsObject.cpp \
		ConvexPhysicsObject.cpp \
		PhysicsContactDamageManager.cpp \
		PhysicsContactEffectManager.cpp \
		PhysicsContactFeedback.cpp \
		PhysicsContactSoundManager.cpp \
		PhysicsContactUtils.cpp \
		RackPhysicsObject.cpp \
		StaticPhysicsObject.cpp \
		TerrainPhysicsObject.cpp \
		gamephysics.cpp \
		physics_none.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
