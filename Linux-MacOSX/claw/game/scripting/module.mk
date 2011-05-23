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
		AnimationScripting.cpp \
		CameraScripting.cpp \
		CinematicScripting.cpp \
		DecorScripting.cpp \
		DevScripting.cpp \
		DirectControlScripting.cpp \
		EnvironmentScripting.cpp \
		GameScripting.cpp \
		GameScriptingUtils.cpp \
		HitChainScripting.cpp \
		ItemScripting.cpp \
		LightScripting.cpp \
		MapScripting.cpp \
		MathScripting.cpp \
		MiscScripting.cpp \
		MissionScripting.cpp \
		OptionScripting.cpp \
		PhysicsScripting.cpp \
		PositionScripting.cpp \
		SoundScripting.cpp \
		StringScripting.cpp \
		SyncScripting.cpp \
		TerrainObjectScripting.cpp \
		TrackingScripting.cpp \
		UnitScripting.cpp \
		WaterScripting.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
