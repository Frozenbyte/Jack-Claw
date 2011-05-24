sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


# process subdirectories
SUBDIRS:= \
		 fmod \
		 null \
		 openal \
		 # empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


# files in this directory
FILES:= \
		AmbientAreaManager.cpp \
		AmplitudeArray.cpp \
		LipsyncManager.cpp \
		LipsyncProperties.cpp \
		MusicPlaylist.cpp \
		SoundLooper.cpp \
		SoundMixer.cpp \
		WaveReader.cpp \
		playlistdefs.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))
SRC_$(d)+=$(SRC_$(d)/$(SOUND))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
