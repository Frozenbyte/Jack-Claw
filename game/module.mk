sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


# process subdirectories
SUBDIRS:= \
		 areas \
		 joints \
		 options \
		 physics \
		 scripting \
		 sync \
		 tracking \
		 # empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


# files in this directory
FILES:= \
		AlienSpawner.cpp \
		AmmoPack.cpp \
		AmmoPackObject.cpp \
		Ani.cpp \
		AniManager.cpp \
		AniRecorder.cpp \
		AniTool.cpp \
		Arm.cpp \
		ArmorUnit.cpp \
		ArmorUnitActor.cpp \
		ArmorUnitType.cpp \
		BlockedHeightAreaFillMapper.cpp \
		BonusManager.cpp \
		Building.cpp \
		BuildingAdder.cpp \
		BuildingList.cpp \
		Bullet.cpp \
		BurningMap.cpp \
		Character.cpp \
		CheckpointChecker.cpp \
		ClawController.cpp \
		ClawUnit.cpp \
		ClawUnitActor.cpp \
		ClawUnitType.cpp \
		ConnectionChecker.cpp \
		CoverFinder.cpp \
		CoverMap.cpp \
		DHLocaleManager.cpp \
		DifficultyManager.cpp \
		DirectWeapon.cpp \
		EngineMetaValues.cpp \
		EnvironmentalEffect.cpp \
		EnvironmentalEffectManager.cpp \
		Flashlight.cpp \
		FoobarAI.cpp \
		Forcewear.cpp \
		Game.cpp \
		GameConfigs.cpp \
		GameMap.cpp \
		GameObject.cpp \
		GameObjectFactoryList.cpp \
		GameObjectList.cpp \
		GameOption.cpp \
		GameOptionManager.cpp \
		GameProfiles.cpp \
		GameProfilesEnumeration.cpp \
		GameRandom.cpp \
		GameScene.cpp \
		GameStats.cpp \
		GameUI.cpp \
		GameWorldFold.cpp \
		Head.cpp \
		HideMap.cpp \
		IndirectWeapon.cpp \
		Item.cpp \
		ItemList.cpp \
		ItemManager.cpp \
		ItemPack.cpp \
		ItemType.cpp \
		Leg.cpp \
		LightBlinker.cpp \
		LineOfJumpChecker.cpp \
		MaterialManager.cpp \
		MissionParser.cpp \
		ObstacleMapUnitObstacle.cpp \
		OptionApplier.cpp \
		Part.cpp \
		PartList.cpp \
		PartType.cpp \
		PartTypeAvailabilityList.cpp \
		PartTypeParser.cpp \
		ParticleSpawner.cpp \
		ParticleSpawnerManager.cpp \
		PlayerPartsManager.cpp \
		PlayerWeaponry.cpp \
		PowerCell.cpp \
		ProgressBar.cpp \
		ProgressBarActor.cpp \
		Projectile.cpp \
		ProjectileActor.cpp \
		ProjectileList.cpp \
		ProjectileTrackerObjectType.cpp \
		Reactor.cpp \
		ReconChecker.cpp \
		SaveData.cpp \
		ScriptDebugger.cpp \
		ScriptableAIDirectControl.cpp \
		SidewaysUnit.cpp \
		SidewaysUnitActor.cpp \
		SlopeTypes.cpp \
		SomeTorso1.cpp \
		SomeTorso1Object.cpp \
		Tool.cpp \
		Torso.cpp \
		UnifiedHandleManager.cpp \
		Unit.cpp \
		UnitActor.cpp \
		UnitFormation.cpp \
		UnitInventory.cpp \
		UnitLevelAI.cpp \
		UnitList.cpp \
		UnitPhysicsUpdater.cpp \
		UnitScriptPaths.cpp \
		UnitSelections.cpp \
		UnitSpawner.cpp \
		UnitTargeting.cpp \
		UnitType.cpp \
		UnitVariables.cpp \
		UnitVisibility.cpp \
		UnitVisibilityChecker.cpp \
		UnreachableAreaFillMapper.cpp \
		UpgradeManager.cpp \
		UpgradeType.cpp \
		VisualObjectModelStorage.cpp \
		Water.cpp \
		WaterManager.cpp \
		Weapon.cpp \
		WeaponObject.cpp \
		createparts.cpp \
		direct_controls.cpp \
		goretypedefs.cpp \
		materials.cpp \
		savegamevars.cpp \
		unittypes.cpp \
		userdata.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES)) $(SRC_$(d)/areas) $(SRC_$(d)/joints) $(SRC_$(d)/options) $(SRC_$(d)/physics) $(SRC_$(d)/scripting) $(SRC_$(d)/tracking)


ALLDIRS+=$(d)


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
