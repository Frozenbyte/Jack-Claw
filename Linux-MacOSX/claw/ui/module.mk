sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


# process subdirectories
SUBDIRS:= \
		 camera_system \
		 # empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


# files in this directory
FILES:= \
		AmbientSound.cpp \
		AmbientSoundManager.cpp \
		AmmoWindow.cpp \
		AmmoWindowCoop.cpp \
		AniRecorderWindow.cpp \
		AnimationSet.cpp \
		Animator.cpp \
		ArmorConstructWindow.cpp \
		ArmorPartSelectWindow.cpp \
		BlackEdgeWindow.cpp \
		CameraAutotilter.cpp \
		CameraAutozoomer.cpp \
		CharacterSelectionWindow.cpp \
		CinematicScreen.cpp \
		CombatMessageWindow.cpp \
		CombatMessageWindowWithHistory.cpp \
		CombatRadar.cpp \
		CombatSubWindowFactory.cpp \
		CombatUnitWindow.cpp \
		CombatWindow.cpp \
		ComboWindow.cpp \
		CreditsMenu.cpp \
		DebugDataView.cpp \
		DebugMapVisualizer.cpp \
		DebugProjectileVisualizer.cpp \
		DebugTerrainObjectVisualizer.cpp \
		DebugTrackerVisualizer.cpp \
		DebugUnitVisualizer.cpp \
		DebugVisualizerTextUtil.cpp \
		DecalPositionCalculator.cpp \
		Decoration.cpp \
		DecorationManager.cpp \
		DynamicLightManager.cpp \
		Ejecter.cpp \
		ElaborateHintMessageWindow.cpp \
		ErrorWindow.cpp \
		FlashlightWindow.cpp \
		GUIEffectWindow.cpp \
		GameCamera.cpp \
		GameConsole.cpp \
		GameController.cpp \
		GamePointers.cpp \
		GameVideoPlayer.cpp \
		GenericBarWindow.cpp \
		GenericTextWindow.cpp \
		HealthWindow.cpp \
		HealthWindowCoop.cpp \
		IngameGuiTabs.cpp \
		ItemWindow.cpp \
		ItemWindowUpdator.cpp \
		JoystickAimer.cpp \
		LightManager.cpp \
		LoadGameMenu.cpp \
		LoadingMessage.cpp \
		LoadingWindow.cpp \
		LogEntry.cpp \
		LogManager.cpp \
		Logwindow.cpp \
		Mainmenu.cpp \
		Map.cpp \
		MapWindow.cpp \
		MenuBaseImpl.cpp \
		MenuCollection.cpp \
		MessageBoxWindow.cpp \
		MissionFailureWindow.cpp \
		MissionSelectionWindow.cpp \
		MovieAspectWindow.cpp \
		Muzzleflasher.cpp \
		NewGameMenu.cpp \
		NoCameraBoundary.cpp \
		OffscreenUnitPointers.cpp \
		OptionsMenu.cpp \
		OptionsWindow.cpp \
		ParticleArea.cpp \
		ParticleCollision.cpp \
		PlayerUnitCameraBoundary.cpp \
		ProfilesMenu.cpp \
		ScoreWindow.cpp \
		SelectionBox.cpp \
		SelectionVisualizer.cpp \
		Spotlight.cpp \
		StorageWindow.cpp \
		TacticalUnitWindow.cpp \
		TargetDisplayButtonManager.cpp \
		TargetDisplayWindow.cpp \
		TargetDisplayWindowButton.cpp \
		TargetDisplayWindowUpdator.cpp \
		TerminalManager.cpp \
		TerminalWindow.cpp \
		Terrain.cpp \
		TerrainCreator.cpp \
		UIEffects.cpp \
		UnitHealthBarWindow.cpp \
		UnitHighlight.cpp \
		UpgradeAvailableWindow.cpp \
		UpgradeWindow.cpp \
		VehicleWindow.cpp \
		Visual2D.cpp \
		VisualEffect.cpp \
		VisualEffectManager.cpp \
		VisualEffectType.cpp \
		VisualObject.cpp \
		VisualObjectModel.cpp \
		WeaponSelectionWindow.cpp \
		WeaponWindow.cpp \
		WeaponWindowCoop.cpp \
		cursordefs.cpp \
		terrain_legacy.cpp \
		uidefaults.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES)) $(SRC_$(d)/camera_system)


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
