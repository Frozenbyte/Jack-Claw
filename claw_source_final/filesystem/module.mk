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
		file_list.cpp \
		file_package_manager.cpp \
		input_compressed_file_stream.cpp \
		input_file_stream.cpp \
		input_stream.cpp \
		input_stream_wrapper.cpp \
		memory_stream.cpp \
		output_file_stream.cpp \
		output_stream.cpp \
		rle_packed_file_wrapper.cpp \
		standard_package.cpp \
		zip_package.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
