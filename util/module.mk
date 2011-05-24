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
		AI_PathFind.cpp \
		AreaMap.cpp \
		BlinkerManager.cpp \
		BuildingBlinker.cpp \
		BuildingHandler.cpp \
		BuildingMap.cpp \
		CheckedIntValue.cpp \
		Checksummer.cpp \
		CircleAreaTracker.cpp \
		ClippedCircle.cpp \
		ColorMap.cpp \
		CursorRayTracer.cpp \
		Dampers.cpp \
		Debug_MemoryManager.cpp \
		DecalManager.cpp \
		DecalSpawner.cpp \
		DecalSystem.cpp \
		DirectionalLight.cpp \
		DistanceFloodfill.cpp \
		FBCopyFile.cpp \
		Floodfill.cpp \
		FogApplier.cpp \
		GridOcclusionCuller.cpp \
		HelperPositionCalculator.cpp \
		Intersect.cpp \
		LightAmountManager.cpp \
		LightMap.cpp \
		LineAreaChecker.cpp \
		LipsyncManager.cpp \
		LocaleManager.cpp \
		LocaleResource.cpp \
		ModelTextureUncompress.cpp \
		ObjectDurabilityParser.cpp \
		Parser.cpp \
		PathDeformer.cpp \
		PathSimplifier.cpp \
		Preprocessor.cpp \
		ScreenCapturer.cpp \
		Script.cpp \
		ScriptManager.cpp \
		ScriptProcess.cpp \
		SelfIlluminationChanger.cpp \
		SimpleParser.cpp \
		SoundMaterialParser.cpp \
		SpotLightCalculator.cpp \
		StringUtil.cpp \
		TextFileModifier.cpp \
		TextFinder.cpp \
		TextureCache.cpp \
		TextureSwitcher.cpp \
		UInt96Hash.cpp \
		UberCrypt.cpp \
		UnicodeConverter.cpp \
		assert.cpp \
		hiddencommand.cpp \
		jpak.cpp \
		mod_selector.cpp \
		procedural_applier.cpp \
		procedural_properties.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
