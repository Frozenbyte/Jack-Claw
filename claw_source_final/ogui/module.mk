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
		Ogui.cpp \
		OguiAligner.cpp \
		OguiButton.cpp \
		OguiButtonEvent.cpp \
		OguiButtonStyle.cpp \
		OguiCheckBox.cpp \
		OguiEffectEvent.cpp \
		OguiException.cpp \
		OguiFormattedCommandImg.cpp \
		OguiFormattedCommandImpl.cpp \
		OguiFormattedText.cpp \
		OguiLocaleWrapper.cpp \
		OguiSelectList.cpp \
		OguiSelectListEvent.cpp \
		OguiSelectListStyle.cpp \
		OguiSlider.cpp \
		OguiStormDriver.cpp \
		OguiTextLabel.cpp \
		OguiTypeEffectListener.cpp \
		OguiWindow.cpp \
		orvgui2.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
