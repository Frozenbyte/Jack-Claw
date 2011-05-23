
#ifndef UNIT_H
#define UNIT_H

#include <DatatypeDef.h>
#include <string>

#include "c2_aabb.h"
#include "gamedefs.h"
#include "GameObject.h"
#include "UnitScriptPaths.h"
#include "UnitTargeting.h"
#include "UnitVariables.h"
#include "UnitVisibility.h"
#include "../ui/VisualObject.h"
#include "../ui/IPointableObject.h"
#include "../ui/IAnimatable.h"
#include "../util/ITrackable.h"
#include "../util/DirectionalLight.h"

// how many previous unit positions are kept in memory
// note, see unit.cpp for the position sampling interval
#define UNIT_BACKTRACK_AMOUNT 8


#define UNIT_MAX_WEAPONS 24
#define UNIT_MAX_AMMOS 16

#define UNIT_MAX_ITEMS 64

#define UNIT_MAX_BLEND_ANIMATIONS 3


namespace frozenbyte
{
	namespace ai
	{
		class Path;
	}
}

struct Storm3D_CollisionInfo;

namespace ui
{
	class AnimationSet;
	class Spotlight;
	class VisualEffect;
}

namespace game
{
	class UnitListEntity;

	extern const int unitDataId;

	class UnitType;
	class Weapon;
	class WeaponObject;
	class Part;
	class AmmoPack;
	class AmmoPackObject;
	class Character;
	class Item;
	class Flashlight;
	class Bullet;
	class AbstractPhysicsObject;
	class PartType;
	struct FootStepTrigger;
	class IAIDirectControl;

	// NOTE: I'm not too happy about this dependency :(
	// Unit <--> Projectile
	class Projectile;


	/**
	 *
	 * A movable unit (entity).
	 * Used mostly as data storage. The act method that moves the unit
	 * based on it's type is in seperate UnitActor class. This structure
	 * is to keep the Unit class independent of game classes (Game).
	 * A unit must always be initialized with a specific owner.
	 * If the unit should not owned by any player, NO_UNIT_OWNER should be used
	 * instead of player number to do that.
	 *
	 * @version 1.1, 9.12.2002
	 * @author Jukka Kokkonen <jukka@frozenbyte.com>
	 * @see UnitActor
	 * @see UnitList
	 * @see Part
	 *
	 */

	class Unit : public GameObject, public ui::IPointableObject, 
		public ui::IVisualObjectData, public ui::IAnimatable,
		public util::ITrackable
	{

	private:
		int walkDelay;
		int fireWaitDelay[UNIT_MAX_WEAPONS];
		int fireReloadDelay[UNIT_MAX_WEAPONS];

	public:
	
		enum UNIT_MODE
		{
			UNIT_MODE_AGGRESSIVE = 1,
			UNIT_MODE_DEFENSIVE = 2,
			UNIT_MODE_HOLD_FIRE = 3,
			UNIT_MODE_KEEP_TARGET = 4
		};

		enum UNIT_SPEED
		{
			UNIT_SPEED_CRAWL = 1,
			UNIT_SPEED_SLOW = 2,
			UNIT_SPEED_FAST = 3,
			UNIT_SPEED_SPRINT = 4,
			UNIT_SPEED_JUMP = 5
		};

		enum UNIT_MOVE_STATE
		{
			UNIT_MOVE_STATE_NORMAL = 1,
			UNIT_MOVE_STATE_STAGGER_BACKWARD = 2,
			UNIT_MOVE_STATE_STAGGER_FORWARD = 3,
			UNIT_MOVE_STATE_IMPACT_BACKWARD = 4,
			UNIT_MOVE_STATE_IMPACT_FORWARD = 5,
			UNIT_MOVE_STATE_IDLE = 6,
			UNIT_MOVE_STATE_UNCONSCIOUS = 7,
			UNIT_MOVE_STATE_UNCONSCIOUS_RISING_UP = 8,
			UNIT_MOVE_STATE_FLIPPING_SIDE = 9,
			UNIT_MOVE_STATE_GO_PRONE = 10,
			UNIT_MOVE_STATE_RISE_PRONE = 11,
			UNIT_MOVE_STATE_STAGGER_LEFT = 12,
			UNIT_MOVE_STATE_STAGGER_RIGHT = 13,
			UNIT_MOVE_STATE_ELECTRIFIED = 14,
			UNIT_MOVE_STATE_STUNNED = 15
		};

		enum UNIT_EFFECT_LAYER
		{
			UNIT_EFFECT_LAYER_NONE = 1,
			UNIT_EFFECT_LAYER_ELECTRIC = 2,
			UNIT_EFFECT_LAYER_BURNING = 3,
			UNIT_EFFECT_LAYER_SLIME = 4,
			UNIT_EFFECT_LAYER_CLOAK = 5,
			UNIT_EFFECT_LAYER_CLOAKHIT = 6,
			UNIT_EFFECT_LAYER_PROTECTIVESKIN = 7,
			UNIT_EFFECT_LAYER_CLOAKRED = 8
		};

		enum UNIT_DIRECT_CONTROL_TYPE
		{
			UNIT_DIRECT_CONTROL_TYPE_NONE = 0,
			UNIT_DIRECT_CONTROL_TYPE_AI = 1,
			UNIT_DIRECT_CONTROL_TYPE_LOCAL_PLAYER = 2,
			UNIT_DIRECT_CONTROL_TYPE_REMOTE_PLAYER = 3
		};

		// psd

		enum MoveType
		{
			MoveTypeNormal = 0,
			MoveTypeFast = 1,
			MoveTypeStealth = 2
		};

		enum FireType
		{
			FireTypePrimary = 0,
			FireTypeSecondary = 1,
			FireTypeBasic = 2,
			FireTypeHeavy = 3,
			FireTypeAll = 4,
		};

		/** 
		 * Base class initialization.
		 * Usually you should create instances of subclasses, not this class.
		 */
		Unit();
		//Unit(int owner);

		virtual ~Unit();

		/** 
		 * TODO!
		 * To implement GameObject "interface" class.
		 * Should return the SaveData object containing all necessary
		 * information about the unit (parts are children).
		 * @return SaveData, data to be saved. TODO, currently NULL.
		 */
		virtual SaveData *getSaveData() const;

		virtual const char *getStatusInfo() const;

		virtual void *getVisualObjectDataId();

		/** 
		 * This method is here to implement IPointerObject interface.
		 * That allows the units to be tracked by the 3d game pointers.
		 * (For targeting enemy units and marking selected units.) 
		 * @return VC3, the world position where any pointer tracking this
		 * unit should be placed.
		 */
		virtual const VC3 &getPointerPosition();

		// to get the actual unit middle position...
		virtual const VC3 getPointerMiddleOffset();

		/**
		 * Returns the root part of the unit.
		 * @return Part*, unit's root part (usually some torso for armors).
		 * Return value is NULL if the unit has no root part.
		 */
		Part *getRootPart();

		/** 
		 * Set the root part of the unit (armor's root is usually a torso).
		 * Before setting a part as a part of the unit, the part should have
		 * been removed from the players storage list (that is, if it was
		 * previously put there). Also, proper handling of the previous
		 * root part (and it's children) should be done before setting a new 
		 * root part. Set to NULL if you don't want the unit to have any parts.
		 * Generally it is a better idea to use ready made methods for handling
		 * unit parts than to modify them directly (see Game::detachParts method). 
		 * @param part	Part* to be set as this units root part.
		 * @see Game::detachParts
		 */
		void setRootPart(Part *part);

		/**
		 * Get the character assigned to this unit.
		 * For human player armors, this is the mercenary inside the armor.
		 * If a human owned armor has no character, it should not take part in 
		 * combat. 
		 * @return Character*, the character assigned to this unit or NULL if none.
		 */
		Character *getCharacter();

		/**
		 * Set the character for this unit.
		 * For human player armors, this is the mercenary inside the armor.
		 * If a human owned armor has no character, it should not take part in 
		 * combat. Remember to handle the previous character properly before
		 * assigning a new one (as otherwise you may leak memory).
		 * @return Character*, the character assigned to this unit or NULL if
		 * the unit is to be set to have no character.
		 */
		void setCharacter(Character *character);

		/** 
		 * Returns player owning this unit (who's side this unit is on)
		 * @return int, the number of the player owning this unit or 
		 * NO_UNIT_OWNER if none.
		 */
		//int getOwner();
		inline int getOwner()
		{
			return owner;
		}

		/** 
		 * Tells whether this unit is in the combat or not.
		 * May be out of the game for injury, not having a character or 
		 * having an incomplete armor, etc.
		 * @return boolean, true if unit is active (can take part in combat), 
		 * else false.
		 */
		inline bool isActive()
		{
			return active;
		}

		/** 
		 * Sets the unit active or inactive.
		 * Do NOT change while in combat. Change only while in menus (before
		 * or after combat). 
		 * @param active	bool, true if the unit can take part in combat,
		 * false if the unit won't take part in combat.
		 */
		void setActive(bool active);

		/** 
		 * Returns true if this unit is one of the selected units.
		 * @return bool, true if unit is selected, else false.
		 */
		bool isSelected();

		/** 
		 * Sets the unit selected or unselected.
		 * @param selected	bool, true if unit is selected, false if not.
		 */
		void setSelected(bool selected);

		/** 
		 * Returns unit's current behaviour mode.
		 * @return UNIT_MODE, the unit's behaviour mode.
		 */
		UNIT_MODE getMode();

		/** 
		 * Returns the overall heat of the unit.
		 * @return int, the unit's heat.
		 */
		int getHeat();

		/** 
		 * Returns the maximum overall heat.
		 * @return int, the unit's max heat.
		 */
		int getMaxHeat();

		/** 
		 * Returns the energy of the unit.
		 * @return int, the unit's energy.
		 */
		int getEnergy();

		/** 
		 * Returns the cooling amount of the unit.
		 * @return int, the unit's cooling amount (done every coolrate ticks).
		 */
		int getCoolingAmount();

		/** 
		 * Returns the recharging amount of the unit.
		 * @return int, the unit's recharging amount (done every rechargerate ticks).
		 */
		int getRechargingAmount();

		/** 
		 * Returns the max energy of the unit.
		 * (may have lowered since the beginning of the combat because of 
		 * damage inflicted to power cells.)
		 * @return int, the unit's current max energy.
		 */
		int getMaxEnergy();

		/** 
		 * Returns the starting max energy of the unit.
		 * @return int, the max energy of the unit at the start of the combat.
		 */
		int getStartEnergy();

		// re-calculates overall heat based on heat of the parts
		//int calculateHeat();

		/**
		 * Re-calculates the max energy of the unit based on parts (power-cells)
		 */
		void calculateMaxEnergy();

		/** 
		 * Set start energy (set based on max energy on combat start-up).
		 * That is, first calculateMaxEnery(), then setStartEnergy(getMaxEnergy())
		 * @param startEnergy  int, the energy the unit has at the beginning of combat
		 */
		void setStartEnergy(int startEnergy);

		/**
		 * Set energy
		 * @param energy	int, amount of energy
		 */
		void setEnergy(int energy);

		/** 
		 * Set heat
		 * @param heat	int, amount of heat
		 */
		void setHeat(int heat);

		/**
		 * Re-calculates the max heat of the unit based on parts
		 */
		void calculateMaxHeat();

		/**
		 * Re-calculates the cooling amount of the unit based on parts
		 */
		void calculateCoolingAmount();

		/**
		 * Re-calculates the recharging rate of the unit based on parts
		 */
		void calculateRechargingAmount();

		/**
		 * TODO: should calculate based on parts?
		 * set max heat 
		 * @param maxHeat  int, maximum heat for the unit.
		 */
		//void setMaxHeat(int maxHeat);

		void calculateWeight();

		int getWeight();

		void calculateRunningValue();
		void calculateStealthValue();
		void calculateReconValue();

		int getRunningValue();
		int getStealthValue();
		int getReconValue();

		bool isUnitMirrorSide();

		void setUnitMirrorSide(bool mirrorSide);

		/** 
		 * Sets the unit's behaviour mode.
		 * @param mode	UNIT_MODE, unit's mode.
		 */
		void setMode(UNIT_MODE mode);

		/**
		 * DEPRECATED!
		 * Use getUnitType method instead!
		 * Returns the type id number of the unit.
		 * A proper unit actor is selected based on this id number.
		 * @return int, the type id number of the unit.
		 */
		int getUnitTypeId();

		void setUnitTypeId(int id);

		/**
		 * Returns the type of the unit.
		 * @return UnitType, the type of the unit.
		 */
		inline UnitType *getUnitType() { return unitType; }

		void setUnitType(UnitType *unitType);

		void setPosition(const VC3 &position);
		void setRotation(float xAngle, float yAngle, float zAngle);
		inline void setVelocity(const VC3 &velocity) { assert(velocity.y >= -2.0f && velocity.y <= 2.0f); this->velocity = velocity; }

		/**
		 * Sets waypoint position for the unit. 
		 * Waypoint coordinates unit (x, y, z) = map (x, height, y).
		 */
		inline void setWaypoint(const VC3 &waypoint) { this->waypoint = waypoint; }
		
		/**
		 * Sets final destination position for the unit. 
		 * Destination coordinates unit (x, y, z) = map (x, height, y).
		 */
		inline void setFinalDestination(const VC3 &finalDestination) 
		{ 
			this->finalDestination = finalDestination; 
		}

		/**
		 * Sets a unit that this unit will see after next visibility update.
		 * @param seeUnit Unit*, the unit that this one is "looking at".
		 */
		void setToBeSeenUnit(Unit *seeUnit);

		Unit *getToBeSeenUnit();

		void setToBeSeenUnitDistance(float distance);
		
		float getToBeSeenUnitDistance();

		bool isAnimated();
		void setAnimated(bool animated);

		/**
		 * Updates the next unit to be seen as current unit being looked at.
		 */
		void useToBeSeenUnit();

		/**
		 * Returns the unit that this unit is currently seeing (looking at).
		 * @return Unit*, pointer to unit seen or NULL if no units seen.
		 */
		inline Unit *getSeeUnit()
		{
			return seeingUnit;
		}

		float getSeeUnitDistance();

		/**
		 * Checks whether unit is at it's waypoint.
		 * @return bool, true if unit has reached it's waypoint, else false.
		 */
		bool atWaypoint();

		/**
		 * Checks whether unit is at it's final destination.
		 * @return bool, true if unit has reached it's destination, else false.
		 */
		bool atFinalDestination();

		/**
		 * Deprecated.
		 */
		void setRotateYSpeed(int rotateYSpeed);

		/**
		 * Deprecated.
		 */
		int getRotateYSpeed();

		/**
		 * Returns the visualization of this unit.
		 * @return VisualObject*, the visualization or NULL if unit is not
		 * visualized.
		 */
		//virtual ui::VisualObject *getVisualObject();
		ui::VisualObject *getVisualObject()
		{
			return visualObject;
		}


		/**
		 * Sets the visualization of this unit.
		 * @param visualObject VisualObject*, the visualization to be set
		 * for this unit or NULL if unit is not to be visualized.
		 * Remember to handle the previous visualObject assigned to the unit
		 * properly before setting a new one (or you may leak memory).
		 */
		void setVisualObject(ui::VisualObject *visualObject);

		inline const VC3 &getPosition() const { return position; }
		inline const VC3 &getVelocity() const { return velocity; }
		inline VC3 getRotation() const { return VC3(xAngle, yAngle, zAngle); }
		inline const VC3 &getWaypoint() const { return waypoint; }
		inline const VC3 &getFinalDestination() const { return finalDestination; }
		//QUAT getRotation();

		virtual int getAnimation();
		virtual void setAnimation(int animation);

		virtual int getBlendAnimation(int num);
		virtual void setBlendAnimation(int num, int animation);

		virtual void setAnimationSpeedFactor(float speedFactor);
		virtual float getAnimationSpeedFactor();

		int getAnimationTimeLeft();
		void setAnimationTimeLeft(int animationTimeLeft);

		inline int getWalkDelay()
		{
			return walkDelay; 
		}
		void setWalkDelay(int delayTicks);
		inline int getFireReloadDelay(int weapon)
		{
			return fireReloadDelay[weapon]; 
		}
		void setFireReloadDelay(int weapon, int delayTicks);
		inline int getFireWaitDelay(int weapon)
		{
			return fireWaitDelay[weapon]; 
		}
		void setFireWaitDelay(int weapon, int delayTicks);

		// call when starting combat
		void initWeapons();

		// call after combat is over
		void uninitWeapons();

		// returns the first part it happens to find (in unspecified order), or null if no part of such part type
		Part *seekPartOfPartType(PartType *partType);

		inline Weapon *getWeaponType(int weapon) { return weaponType[weapon]; }
		void setWeaponType(int weapon, Weapon *weaponType);
		int getWeaponPosition(int weapon);
		int getWeaponAmmoAmount(int weapon);
		int getWeaponMaxAmmoAmount(int weapon);
		AmmoPack *getWeaponAmmoType(int weapon);


		Weapon *getCustomizedWeaponType(Weapon *weapon);
		void deleteCustomizedWeaponTypes();
		Weapon *createCustomizedWeaponType(Weapon *weapon);

		inline Projectile *getWeaponCopyProjectile(int weapon) { return weaponCopyProjectile[weapon]; }
		void setWeaponCopyProjectile(int weapon, Projectile *projectile);

		void setWeaponFireTime(int weapon, int delayTicks);

		inline int getWeaponFireTime(int weapon) { return weaponFireTime[weapon]; }

		float getMaxWeaponRange();

		bool hasAnyWeaponReady();

		// returns false if was out of ammo
		bool useWeaponAmmo(int weapon);

		// returns false, if no reload was done 
		// (out of ammo or clip already full)
		bool reloadWeaponAmmoClip(int weapon, bool instantReload = false);

		int getWeaponAmmoInClip(int weapon);

		void clearWeaponAmmoInClip(int weapon);

		// weapon selected in use
		// (unoperable weapons are always inactive too..?)
		bool isWeaponActive(int weapon);
		void setWeaponActive(int weapon, bool active);

		// weapon operable (not out of ammo or broken)
		bool isWeaponOperable(int weapon);
		void setWeaponOperable(int weapon, bool operable);

		// weapon model visible
		bool isWeaponVisible(int weapon);
		void setWeaponVisible(int weapon, bool visible);

		// sets each weapon active or inactive based on the given firetype
		void setWeaponsActiveByFiretype(FireType fireType);

		// inactivated anti vehicle purpose rockets, etc.
		void inactivateAntiVehicleWeapons();

		// returns true if unit has some weapons for the given firetype
		bool hasWeaponsForFiretype(FireType fireType);

		int getWeaponLoopSoundHandle(int weapon);
		int getWeaponLoopSoundKey(int weapon);
		void setWeaponLoopSoundHandle(int weapon, int handle, int key);

		int getWeaponSoundHandle(int weapon);
		void setWeaponSoundHandle(int weapon, int handle);

		void setSelectedWeapon(int weaponNumber);
		int getSelectedWeapon();

		void setSelectedSecondaryWeapon(int weaponNumber);
		int getSelectedSecondaryWeapon();

		int getWeaponByWeaponType(int weaponTypeId);

		//bool isDestroyed();
		bool isDestroyed()
		{
			return destroyed;
		}

		void setDestroyed(bool destroyed);

		void setPath(frozenbyte::ai::Path *path);

		frozenbyte::ai::Path *getPath();

		void setPathIndex(int pathIndex);
		int getPathIndex();
		bool isAtPathEnd();

		int getBlockedTime();
		void increaseBlockedTime();
		void clearBlockedTime();

		int getNonBlockedTime();
		void increaseNonBlockedTime();
		void clearNonBlockedTime();

		void setAI(void *ai);
		void *getAI();

		bool isOnGround();
		void setOnGround(bool onGround);

		bool isGroundFriction();
		void setGroundFriction(bool groundFriction);

		char *getScript();
		void setScript(char *scriptName);

		Unit *getLeader();
		void setLeader(Unit *unit);

		void restoreDefaultSpeed();
		void setSpeed(UNIT_SPEED speed);
		inline Unit::UNIT_SPEED getSpeed()
		{
			return speed;
		}

		void setMoveState(UNIT_MOVE_STATE moveState);
		Unit::UNIT_MOVE_STATE getMoveState();
		void setMoveStateCounter(int moveStateCounter);
		int getMoveStateCounter();

		float getAngleTo(const VC3 &toPosition);

		void setTurnToAngle(float angle);
		bool isTurning();
		void stopTurning();
		float getTurnToAngle();

		void setSpottedScriptDelay(int delayTicks);
		int getSpottedScriptDelay();

		void setHitScriptDelay(int delayTicks);
		int getHitScriptDelay();
		void setHitByUnit(Unit *unit, Bullet *hitBullet);
		Unit *getHitByUnit();
		Bullet *getHitByBullet();

		void setHitMissScriptDelay(int delayTicks);
		int getHitMissScriptDelay();
		void setHitMissByUnit(Unit *unit);
		Unit *getHitMissByUnit();

		void setPointedScriptDelay(int delayTicks);
		int getPointedScriptDelay();
		void setPointedByUnit(Unit *unit);
		Unit *getPointedByUnit();

		void setHearNoiseScriptDelay(int delayTicks);
		int getHearNoiseScriptDelay();
		void setHearNoiseByUnit(Unit *unit);
		Unit *getHearNoiseByUnit();

		void setSpawnCoordinates(VC3 &spawn);
		VC3 getSpawnCoordinates();
		void clearSpawnCoordinates();
		bool hasSpawnCoordinates();

		void setIdleTime(int idleTime);
		int getIdleTime();

		void setLookBetaAngle(float lookBetaAngle);
		float getLookBetaAngle();

		bool isDirectControl();
		void setDirectControl(bool directControl);
		void setDirectControlType(UNIT_DIRECT_CONTROL_TYPE directControlType);
		UNIT_DIRECT_CONTROL_TYPE getDirectControlType();

		int getGroupNumber();
		void setGroupNumber(int groupNumber);

		int getHP();
		void setHP(int hp);

		int getMaxHP();
		void setMaxHP(int maxHP);

		float getPoisonResistance();
		void setPoisonResistance(float value);

		float getCriticalHitPercent();
		void setCriticalHitPercent(float value);

		void enableHPGain(float limit, int amount, int delay, int startdelay, float damagefactor);
		void disableHPGain(void);
		float getHPGainLimit(void);
		int getHPGainAmount(void);
		int getHPGainDelay(void);
		int getHPGainStartDelay(void);
		int getHPGainStartTime(void);
		float getHPGainDamageFactor(void);
		int getLastTimeDamaged(void);
		void setLastTimeDamaged(int time);
		void setHPGainStartTime(int time);

		void setTurningSound(int handle, int timeNow);
		int getTurningSound();
		int getTurningSoundStartTime();
		int getTurningSoundVolume();
		int getTurningSoundFrequency();
		void setTurningSoundVolume(int vol);
		void setTurningSoundFrequency(int freq);

		int calculateArmorRating();

		void setSightBonus(bool sightBonus);
		bool hasSightBonus();

		void useStoredPath(int pathNumber);

		void setStoredPath(int pathNumber, frozenbyte::ai::Path *path,
			const VC3 &startPosition, const VC3 &endPosition);

		void prepareAnimationSet();
		ui::AnimationSet *getAnimationSet();

		void setLastBoneAimDirection(float angle);
		float getLastBoneAimDirection();

		void setLastBoneAimBetaAngle(float angle);
		float getLastBoneAimBetaAngle();

		void setFlashlightDirection(float angle);
		float getFlashlightDirection();

		void setStealthing(bool stealthing);
		bool isStealthing();

		bool isStealthVisualInUse();
		void setStealthVisualInUse(bool useStealth);

		void setReconAvailableFlag(bool reconAvailable);
		bool isReconAvailableFlag();

		void setFallenOnBack(bool fallenOnBack);
		bool hasFallenOnBack();

		float getFiringSpreadFactor();
		void setFiringSpreadFactor(float firingSpreadFactor);

		void setMovingForward();
		void setMovingBackward();
		void setMovingSideways();

		bool wasMovingForward();
		bool wasMovingBackward();
		bool wasMovingSideways();

		float getRushDistance();
		void setRushDistance(float rushDistance);
	
		// call for every non-sneaky step taken by the unit,
		// returns true if a noise is to be made for this step, else false
		// (noises mean gameplay noises, not actual sounds)
		bool makeStepNoise();

		void retireFromCombat();

		void setArmorAmount(int value);
		int getArmorAmount();
		void setArmorClass(int value);
		int getArmorClass();

		void setMuzzleflashVisualEffect(ui::VisualEffect *visualEffect, int muzzleflashDuration);
		void advanceMuzzleflashVisualEffect();
		bool isMuzzleflashVisible();
		void resetMuzzleflashDuration(int muzzleflashDuration, int atTime = 0);
		ui::VisualEffect *getMuzzleflashVisualEffect();

		// HACK: this is kinda hacky way to implement eject
		void setEjectVisualEffect(ui::VisualEffect *visualEffect, int ejectDuration);
		void advanceEjectVisualEffect();
		bool isEjectVisible();
		void resetEjectDuration(int ejectDuration, int atTime = 0);
		ui::VisualEffect *getEjectVisualEffect();

		// laser pointer thing
		void setPointerVisualEffect(ui::VisualEffect *visualEffect);
		void setPointerHitVisualEffect(ui::VisualEffect *visualEffect);
		ui::VisualEffect *getPointerVisualEffect(void);
		ui::VisualEffect *getPointerHitVisualEffect(void);

		//void setSpotlight(ui::Spotlight *spotlight);
		//ui::Spotlight *getSpotlight();

		// TODO: halo class instead of direct spotlight usage!
		void setSecondarySpotlight(ui::Spotlight *spotlight);
		ui::Spotlight *getSecondarySpotlight();

		bool isClipReloading();

		// returns true if ammo was added, false if cannot take that
		// ammo, or ammo amount full.
		bool addWeaponAmmo(AmmoPack *ammoPackType, int amount);
		bool setWeaponAmmo(AmmoPack *ammoPackType, int amount);

		// WARNING: not really "strafed", but "45 angle instead of 90"
		bool isLastMoveStrafed();
		void setLastMoveStrafed(bool lastMoveStrafed);

		void setForcedAnimation(int anim);
		int getForcedAnimation();

		void addItem(int number, Item *item);
		Item *getItem(int number);
		Item *removeItem(int number);
		int getItemNumberByTypeId(int itemTypeId);

		Flashlight *getFlashlight();
		void setFlashlight(Flashlight *flashlight);

		bool wasLastPathfindSuccess();
		void setLastPathfindSuccess(bool pathfindSuccess);

		void setImmortal(bool immortal, bool run_hitscript = false);
		bool isImmortal();
		bool isImmortalWithHitScript();

		void setJumpCounter(int jumpTickLength);
		int getJumpCounter();
		void setJumpTotalTime(int jumpTotalTime);
		int getJumpTotalTime();

		void setTurnedTime(int turnedTime);
		int getTurnedTime();

		void setDeathBleedDelay(int bleedDelay);
		int getDeathBleedDelay();

		void setFiringInProgress(bool firingInProgress);
		bool isFiringInProgress();

		void addPositionToBacktrack(const VC3 &position, const VC3 &oldPosition);
		bool hasMovedSinceOldestBacktrack(float movementTreshold);
		bool hasAttemptedToMoveAllBacktrackTime();

		const char *getIdString();
		void setIdString(const char *idstring);

		int getIdNumber() const;
		void setIdNumber(int id);

		const char *getExecuteTipText();
		void setExecuteTipText(const char *executeTip);

		void setStrafeRotationOffset(float rotationOffset);
		float getStrafeRotationOffset();
		void setStrafeAimOffset(float aimOffset);
		float getStrafeAimOffset();

		void setCameraRelativeJumpDirections(bool forward, bool backward, bool left, bool right);
		void getCameraRelativeJumpDirections(bool *forward, bool *backward, bool *left, bool *right);
		void setUnitRelativeJumpDirections(bool forward, bool backward, bool left, bool right);
		void getUnitRelativeJumpDirections(bool *forward, bool *backward, bool *left, bool *right);
		void setJumpAnim(int anim);
		int getJumpAnim();

		int getPendingWeaponChangeRequest();
		void setPendingWeaponChangeRequest(int pendingWeaponId);

		int getJumpNotAllowedTime();
		void setJumpNotAllowedTime(int jumpNotAllowedTime);

		void makeGhostOfFuture();
		bool isGhostOfFuture();
		int getGhostTime();
		void setGhostTime(int ghostTime);

		int getWeaponForSharedClip(int weapon);
		int getAttachedWeapon(int weapon);

		void addSlowdown(float slowdownFactor);
		void setSlowdown(float slowdownFactor);
		float getSlowdown() const;
		void wearOffSlowdown();

		void setUnitEffectLayerType(UNIT_EFFECT_LAYER layerType, int duration);
		void setUnitEffectLayerDuration(int duration);
		UNIT_EFFECT_LAYER getUnitEffectLayerType() const;
		int getUnitEffectLayerDuration() const;

		void setIdleRequest(int idleRequestAnimNumber);
		int getIdleRequest();
		void clearIdleRequest();

		bool doesKeepReloading();
		int getKeepFiringCount();
		void setKeepReloading(bool keepReloading);
		void setKeepFiringCount(int keepFiringCount);

		bool doesUseAIDisableRange();
		void setUseAIDisableRange(bool aiDisableRange);

		void setSelectedItem(int selectedItem);
		int getSelectedItem();

		void advanceFade();

		float getCurrentVisibilityFadeValue();
		void fadeVisibility(float fadeToValue, int fadeTime);
		void setFadeVisibilityImmediately(float fadeValue);

		void setLightVisibilityFactor(float lightVisibility, bool forceLightVisibilityFactor);
		float getLightVisibilityFactor();
		bool isForcedLightVisibilityFactor();

		float getCurrentLightingFadeValue();
		void fadeLighting(float fadeToValue, int fadeTime);

		bool doesCollisionCheck();
		void setCollisionCheck(bool collCheck);

		bool doesCollisionBlockOthers();
		void setCollisionBlocksOthers(bool enabled);

		bool hasAreaTriggered();
		void setAreaTriggered(bool areaTriggered);

		virtual VC3 getTrackablePosition() const;
		virtual float getTrackableRadius2d() const;

		VC3 &getAreaCenter();
		void setAreaCenter(const VC3 &position);

		float getAreaRange();
		void setAreaRange(float range);

		int getAreaClipMask();
		void setAreaClipMask(int clipMask);

		int getAreaCircleId();
		void setAreaCircleId(int areaCircleId);

		bool isFollowPlayer();
		void setFollowPlayer(bool followPlayer);

		util::DirectionalLight &getDirectionalLight() { return directionalLight; }
		const util::DirectionalLight &getDirectionalLight() const { return directionalLight; }

		void setLastLightUpdatePosition(const VC3 &position);
		VC3 getLastLightUpdatePosition();

		void increaseContinuousFireTime();
		void clearContinuousFireTime();
		int getContinuousFireTime();

		bool isOnFire();
		int getOnFireCounter();
		void setOnFireCounter(int value);

		bool hasAliveMarker();
		void setAliveMarker(bool alive);

		int getWeaponClipSize(int weapon);

		bool isAnyWeaponFiring();

		int getFireSweepDirection();
		void setFireSweepDirection(int sweepDirection);

		bool hasDiedByPoison();
		void setDiedByPoison(bool diedByPoison);

		void rotateWeaponBarrel(int weapon);
		int getRotatedWeaponBarrel(int weapon);

		void rotateWeaponEjectBarrel(int weapon);
		int getRotatedWeaponEjectBarrel(int weapon);

		bool isShootAnimStanding();
		void setShootAnimStanding(bool shootAnimStanding);

		bool isRushing();
		void setRushing(bool rushing);

		//void setDelayedHitProjectileBullet(Bullet *bullet, int interval, int amount);

		void setDelayedHitProjectileBullet(Bullet *bullet);
		void setDelayedHitProjectileInterval(int interval);
		void setDelayedHitProjectileAmount(int amount);

		Bullet *getDelayedHitProjectileBullet();
		int getDelayedHitProjectileInterval();
		int getDelayedHitProjectileAmount();

		void setAniRecordBlendEndFlag(bool blendEndFlag);
		void setAniRecordBlendFlag(int blendAnim);

		bool getAniRecordBlendEndFlag();
		int getAniRecordBlendFlag();

		bool hasAniRecordFireFlag();
		VC3 getAniRecordFireSourcePosition();
		VC3 getAniRecordFireDestinationPosition();
		void setAniRecordFirePosition(const VC3 &sourcePosition, const VC3 &targetPosition);
		void clearAniRecordFireFlag();

		void setWeaponRechargeAmount(int chargeAmount);
		int getWeaponRechargeAmount();
		bool isWeaponRecharging();
		void setWeaponRecharging(bool charging);
		void setWeaponRechargeRange(int chargeMin, int chargePeak, int steps);
		int getWeaponRechargeMin();
		int getWeaponRechargePeak();
		int getWeaponRechargeSteps();

		void setDisappearCounter(int counterValue) { this->disappearCounter = counterValue; }
		int getDisappearCounter() { return this->disappearCounter; }

		void setHitAnimationCounter(int counterValue) { this->hitAnimationCounter = counterValue; }
		int getHitAnimationCounter() { return this->hitAnimationCounter; }

		void setDestroyedTime(int counterValue) { this->destroyedTime = counterValue; }
		int getDestroyedTime() { return this->destroyedTime; }

		void setHitAnimationBoneAngle(float value) { this->hitAnimationBoneAngle = value; }
		float getHitAnimationBoneAngle() { return this->hitAnimationBoneAngle; }

		void setHitAnimationVector(const VC3 &value) { this->hitAnimationVector = value; }
		VC3 getHitAnimationVector() { return this->hitAnimationVector; }

		void setHitAnimationFactor(float value) { this->hitAnimationFactor = value; }
		float getHitAnimationFactor() { return this->hitAnimationFactor; }

		void setSpottable(bool spottable) { this->spottable = spottable; }
		bool isSpottable() { return this->spottable; }

		void setClipEmptySoundDone(bool snddone) { this->clipEmptySoundDone = snddone; }
		bool isClipEmptySoundDone() { return this->clipEmptySoundDone; }

		void setClipEmptyTime(int value) { this->clipEmptyTime = value; }
		int getClipEmptyTime() { return this->clipEmptyTime; }

		void setAimStopCounter(int value) { this->aimStopCounter = value; }
		int getAimStopCounter() { return this->aimStopCounter; }

		void setBurnedCrispyAmount(int value) { this->burnedCrispyAmount = value; }
		int getBurnedCrispyAmount() { return this->burnedCrispyAmount; }

		void setLastRotationDirection(int value) { this->lastRotationDirection = value; }
		int getLastRotationDirection() { return this->lastRotationDirection; }

		void setActCheckCounter(int value) { this->actCheckCounter = value; }
		int getActCheckCounter() { return this->actCheckCounter; }

		void setGamePhysicsObject(AbstractPhysicsObject *obj) { this->physicsObject = obj; }
		AbstractPhysicsObject *getGamePhysicsObject() { return this->physicsObject; }

		void setFluidContainmentPhysicsObject(AbstractPhysicsObject *obj) { this->fluidContainmentPhysicsObject = obj; }
		AbstractPhysicsObject *getFluidContainmentPhysicsObject() { return this->fluidContainmentPhysicsObject; }

		void setActed(bool value) { this->acted = value; }
		bool hasActed() { return this->acted; }

		const std::vector<FootStepTrigger *> &getFootStepTriggers();

		// (sideways)
		void setSideways(bool value) { this->sideways = value; }
		bool isSideways() { return this->sideways; }

		// (sideways)
		void setSideGravityX(float value) { this->sideGravityX = value; }
		float getSideGravityX() { return this->sideGravityX; }

		// (sideways)
		void setSideGravityZ(float value) { this->sideGravityZ = value; }
		float getSideGravityZ() { return this->sideGravityZ; }

		// (sideways)
		void setSideVelocityMax(float value) { this->sideVelocityMax = value; }
		float getSideVelocityMax() { return this->sideVelocityMax; }

		void setAnimationLastPosition(const VC3 &value) { this->animationLastPosition = value; }
		const VC3 &getAnimationLastPosition() { return this->animationLastPosition; }

		// (sideways)
		bool isOnPhysicsObject() { return this->onPhysicsObject; }
		void setOnPhysicsObject(bool value) { this->onPhysicsObject = value; }

		// (sideways)
		bool isOnSlope() { return this->onSlope; }
		void setOnSlope(bool value) { this->onSlope = value; }

		bool isPhysicsObjectFeedbackEnabled() { return this->physicsObjectFeedbackEnabled; }
		void setPhysicsObjectFeedbackEnabled(bool value) { this->physicsObjectFeedbackEnabled = value; }

		bool isPhysicsObjectLock() { return this->physicsObjectLock; }
		void setPhysicsObjectLock(bool value) { this->physicsObjectLock = value; }

		float getPhysicsObjectDifference() { return this->physicsObjectDifference; }
		void setPhysicsObjectDifference(float value) { this->physicsObjectDifference = value; }
		
		// for Survivors snow hack
		void setVisualizationOffsetInterpolation( float value ) { visualizationOffsetInterpolation = value; }
		float getVisualizationOffsetInterpolation() const { return visualizationOffsetInterpolation; }
		static void setVisualizationOffset( float offset );
		static float getVisualizationOffset() { return visualizationOffset; }
		static bool getVisualizationOffsetInUse() { return visualOffsetInUse; }

		// FOR UNITLIST's USE
		void setUnitListEntity(UnitListEntity *listEntity);
		UnitListEntity *getUnitListEntity();

		void setWeaponAmmoInClip(int weapon, int ammoInClip);

		bool increaseEjectRateCounter(int maxValue);


		// --- FOR UNITACTOR's USE... 
		// (optimization, trying to avoid unnecessary float->int casts)
		int obstacleX;
		int obstacleY;
		bool obstacleExists;
		float obstacleAngle;
		bool obstacleOverlaps;
		// ---

		UnitScriptPaths scriptPaths;
		UnitTargeting targeting;
		UnitVariables variables;
		UnitVisibility visibility;

		bool unitMirrorSide;

	// HACK: for unitactor's use
		float lastXRotation;
		float lastZRotation;

		void setOwner(int player);

		float getCustomTimeFactor() { return customTimeFactor; }
		void setCustomTimeFactor(float factor) { customTimeFactor = factor; }

		IAIDirectControl *getAIDirectControl() { return aiDirectControl; }
		void setAIDirectControl(IAIDirectControl *dc) { aiDirectControl = dc; }

		float getLaunchSpeed() const { return launchSpeed; }
		void setLaunchSpeed(float speed) { launchSpeed = speed; }
		bool getLaunchNow() const { return launchNow; }
		void setLaunchNow(bool enabled) { launchNow = enabled; }

		const std::string &getTorsoModelOverride() { return torsoModelOverride; }
		void setTorsoModelOverride(const std::string &str) { torsoModelOverride = str; }

	protected:
		// wonder what these are... they have something to do with the
		// PartTypeParser (used to parse units too), but don't really know
		// what they have to do with Unit class (UnitType seems more wise)
		/*
		bool setRootSub(char *key);
		bool setRootData(char *key, char *value);
		*/

		int unitTypeId;
		UnitType *unitType;

		/**
		 * To help extending classes get the weapons that the unit has.
		 * @param weaponArray  Part **, the array to fill with pointers to weapons.
		 * @param arraySize  int, max weapons to return (size of the array).
		 * @param startPart  Part, the subparts of which are to be looked for weapons
		 * (usually you want to give the unit's root part).
		 * @param startNumber  int, the array index from which to start the filling.
		 * Defaults to 0.
		 * @return int, number of weapons put to array
		 */
		int getWeaponArray(WeaponObject **weaponArray, int arraySize, Part *startPart, int startNumber = 0);

		/**
		 * Like getWeaponArray, but lists ammo packs, not weapons.
		 */
		int getAmmoArray(AmmoPackObject **ammoArray, int arraySize, Part *startPart, int startNumber = 0);

		bool useWeaponAmmoImpl(int weapon, int usageWeapon);

	private:
		Part *rootPart;
		Character *character;

		int owner;

		int idNumber;

		frozenbyte::ai::Path *path;
		int pathIndex;
		bool atPathEnd;
		bool pathIsStored;

		bool active;
		bool selected;
		UNIT_MODE mode;

		bool destroyed;

		int blockedTime;
		int nonBlockedTime;

		int hp;
		int maxHP;

		float poisonResistance;
		float criticalHitPercent;

		// tough guy stuff:
		float hpGainLimit;
		int hpGainAmount;
		int hpGainDelay;
		int hpGainStartDelay;
		int hpGainStartTime;
		float hpGainDamageFactor;
		int lastTimeDamaged;

		int turningSound;
		int turningSoundStartTime;
		int turningSoundVolume;
		int turningSoundFrequency;

		int heat;
		int maxHeat;
		int energy;
		int maxEnergy;
		int startEnergy;

		int coolingAmount;
		int rechargingAmount;

		int weight;

		bool sightBonus;

		int runningValue;
		int stealthValue;
		int reconValue;

		float lastBoneAimDirection;
		float lastBoneAimBetaAngle;
		float flashlightDirection;

		int currentAnimation;
		int currentBlendAnimation[UNIT_MAX_BLEND_ANIMATIONS];
		float animationSpeedFactor;
		int animationTimeLeft;

		Weapon *weaponType[UNIT_MAX_WEAPONS];
		int weaponPosition[UNIT_MAX_WEAPONS];
		bool weaponActive[UNIT_MAX_WEAPONS];
		bool weaponOperable[UNIT_MAX_WEAPONS];
		bool weaponVisible[UNIT_MAX_WEAPONS];
		int weaponAmmoNumber[UNIT_MAX_WEAPONS];
		int weaponFireTime[UNIT_MAX_WEAPONS];
		Projectile *weaponCopyProjectile[UNIT_MAX_WEAPONS];
		int weaponLoopSoundHandle[UNIT_MAX_WEAPONS];
		int weaponLoopSoundKey[UNIT_MAX_WEAPONS];
		int weaponSoundHandle[UNIT_MAX_WEAPONS];
		int weaponAmmoInClip[UNIT_MAX_WEAPONS];
		int weaponBarrel[UNIT_MAX_WEAPONS];
		int weaponEjectBarrel[UNIT_MAX_WEAPONS];

		AmmoPack *ammoType[UNIT_MAX_AMMOS];
		int ammoAmount[UNIT_MAX_AMMOS];
		int maxAmmoAmount[UNIT_MAX_AMMOS];

		struct WeaponCustomization
		{
			int originalId;
			Weapon *weapon;
		};
		std::vector<WeaponCustomization> weaponCustomizations;

		float maxWeaponRange;

		VC3 position;

		// want to handle velocity as components or magnitude and angle?
		// I think components are better, so that's used here for now.
		VC3 velocity;

		VC3 backtrackPositions[UNIT_BACKTRACK_AMOUNT];
		bool backtrackMoves[UNIT_BACKTRACK_AMOUNT];
		int currentBacktrackSlot;
		int backtrackCounter;

		// angles
		// HACK: need to access yAngle for doors...
		float xAngle;
	public:
		float yAngle;

		// stupid stupid hack
		bool moveInReverseDir;

	private:
		bool touchProjectileEnabled;
		float zAngle;

		// temporary waypoint
		VC3 waypoint;

		// final destination
		VC3 finalDestination;

		// angle change speed for y angle
		int rotateYSpeed;

		ui::VisualObject *visualObject;

		//void moveParts(Part *part, VC3 position);
		//void rotateParts(Part *part, float xAngle, float yAngle, float zAngle);

		Unit *seeingUnit;
		Unit *toBeSeeingUnit;

		// these values are not exact, snapshots of the situation when the
		// unit was seen
		float seeingUnitDistance;
		float toBeSeeingUnitDistance;

		bool onGround;
		bool groundFriction;

		void *ai;
		char *script;

		Unit *leader;

		bool turning;
		float turningToAngle;

		UNIT_SPEED speed;

		UNIT_MOVE_STATE moveState;
		int moveStateCounter;

		int spottedDelay;
		int hitDelay;
		int hitMissDelay;
		int pointedDelay;
		int hearNoiseDelay;
		Unit *hitByUnit;
		Unit *hitMissByUnit;
		Unit *pointedByUnit;
		Unit *hearNoiseByUnit;
		Bullet *hitByBullet;

		VC3 spawn;
		bool hasSpawn;

		int idleTime;

		float lookBetaAngle;
		bool directControl;
		UNIT_DIRECT_CONTROL_TYPE directControlType;

		int groupNumber;
		ui::AnimationSet *animationSet;

		int stepNoiseCounter;

		bool stealthing;
		bool stealthVisualInUse;

		bool reconAvailable;

		bool fallenOnBack;

		float firingSpreadFactor;

		bool movingForward;
		bool movingBackward;
		bool movingSideways;

		float rushDistance;

		int armorAmount;
		int armorClass;

		ui::Spotlight *spotlight;
		ui::Spotlight *spotlight2;

		ui::VisualEffect *muzzleflashVisualEffect;
		int muzzleflashDuration;

		ui::VisualEffect *ejectVisualEffect;
		int ejectDuration;

		// laser pointer thing
		ui::VisualEffect *pointerVisualEffect;
		ui::VisualEffect *pointerHitVisualEffect;

		bool deathScriptHasRun;

		bool clipReloading;

		bool lastMoveStrafed;

		int forcedAnim;

		Item **items;

		int selectedWeapon;
		int selectedSecondaryWeapon;

		Flashlight *flashlight;

		bool lastPathfindSuccess;

		bool immortal;
		bool immortalWithHitScript;

		int jumpCounter;
		int jumpTotalTime;
		int turnedTime;

		int deathBleedDelay;

		bool animated;
		bool firingInProgress;

		char *idString;

		float strafeRotationOffset;
		float strafeAimOffset;

		std::vector<FootStepTrigger *> footStepTriggers;

		bool jumpCamForward;
		bool jumpCamBackward;
		bool jumpCamLeft;
		bool jumpCamRight;
		bool jumpUnitForward;
		bool jumpUnitBackward;
		bool jumpUnitLeft;
		bool jumpUnitRight;
		int jumpAnim;

		int jumpNotAllowedTime;

		int pendingWeaponChangeRequest;

		bool ghostOfFuture;
		int ghostTime;

		float slowdown;

		int idleRequest;

		int effectLayerDuration;
		UNIT_EFFECT_LAYER effectLayerType;

		bool keepReloading;
		int keepFiring;

		bool useAIDisableRange;

		int selectedItem;

		float currentLighting;
		float fadeLightingTo;
		int fadeLightingTimeLeft;
		int fadeLightingTimeTotal;

		float currentVisibility;
		float fadeVisibilityTo;
		int fadeVisibilityTimeLeft;
		int fadeVisibilityTimeTotal;

		float lightVisibilityFactor;
		bool forceLightVisibilityFactor;

		bool collisionCheck;
		bool collisionBlocksOthers;
		bool areaTriggered;

		float areaRange;
		int areaClipMask;
		VC3 areaCenter;
		int areaCircleId;

		bool followPlayer;

		VC3 lastLightUpdatePosition;

		util::DirectionalLight directionalLight;

		UnitListEntity *unitListEntity;

		int continuousFireTime;

		int onFireCounter;

		bool aliveMarker;

		int sweepDirection;

		char *executeTipText;

		bool diedByPoison;

		bool shootAnimStanding;

		bool rushing;

		Bullet *delayedHitProjectileBullet;
		int delayedHitProjectileInterval;
		int delayedHitProjectileAmount;

		int aniRecordBlendFlag;
		bool aniRecordBlendEndFlag;

		bool aniRecordFireFlag;
		VC3 aniRecordFireSourcePosition;
		VC3 aniRecordFireDestinationPosition;

		int			highlightStyle;
		std::string highlightText;

		bool weaponCharging;
		int weaponChargeAmount;
		int weaponChargeMin;
		int weaponChargePeak;
		int weaponChargeSteps;

		int disappearCounter;

		int hitAnimationCounter;
		int destroyedTime;

		float hitAnimationBoneAngle;		
		VC3 hitAnimationVector;		
		float hitAnimationFactor;

		bool spottable;

		bool clipEmptySoundDone;
		int clipEmptyTime;

		int aimStopCounter;

		int burnedCrispyAmount;

		int lastRotationDirection;

		AbstractPhysicsObject *physicsObject;
		AbstractPhysicsObject *fluidContainmentPhysicsObject;

		int actCheckCounter;
		bool acted;

		bool sideways;
		float sideGravityX;
		float sideGravityZ;
		float sideVelocityMax;

		VC3 animationLastPosition;
		bool onPhysicsObject;
		bool onSlope;

		int ejectRateCounter;

		bool physicsObjectFeedbackEnabled;
		float physicsObjectDifference;
		bool physicsObjectLock;

		// for survivors snow levels 
		float visualizationOffsetInterpolation;
		static float visualizationOffset;
		static bool visualOffsetInUse;

		float unitListRadius;

		float customTimeFactor;

		// target lock upgrade
		int lastTargetLockTime;
		int targetLockReleaseTime;
		int targetLockCancelTime;
		int targetLockCounter;
		int targetLockCounterMax;
		bool targetLockSoundPlayed;
		Unit *lastTargetLockUnit;

		IAIDirectControl *aiDirectControl;

		int speedBeforeFiring;


		float launchSpeed;
		bool launchNow;

		std::string torsoModelOverride;

		bool shielded;

	public:
		void SphereCollision(const VC3 &position, float radius, Storm3D_CollisionInfo &info, bool) const
		{
			if (this->unitListEntity)
			{
				VC3 diff = this->position - position;
				float maxRadius = unitListRadius + radius;
				if (diff.GetSquareLength() <= maxRadius)
					info.hit = true;
				else
					info.hit = false;
			} else {
				info.hit = true;
			}
		}

		bool fits(const AABB &area) const
		{
			return false;
		}

		void setUnitListRadius(float radius)
		{
			this->unitListRadius = radius;
		}

		// target lock upgrade
		int getLastTargetLockTime(void) { return lastTargetLockTime; }
		void setLastTargetLockTime(int time) { lastTargetLockTime = time; }

		int getTargetLockCounter(void) { return targetLockCounter; }
		void setTargetLockCounter(int value) { targetLockCounter = value; }
		
		void setTargetLockCounterMax(int max) { targetLockCounterMax = max; }
		int getTargetLockCounterMax(void) { return targetLockCounterMax; }

		void setLastTargetLockUnit(Unit *unit) { lastTargetLockUnit = unit; }
		Unit *getLastTargetLockUnit(void) { return lastTargetLockUnit; }

		void setTargetLockTimes(int release, int cancel) { targetLockReleaseTime = release; targetLockCancelTime = cancel; }

		void updateUnitTargetLock(int time, bool being_locked);

		bool wasTargetLockSoundPlayed() const { return targetLockSoundPlayed; }
		void setTargetLockSoundPlayed(bool played) { targetLockSoundPlayed = played; }


		void setSpeedWhileFiring(Unit::UNIT_SPEED speed);
		void resetSpeedAfterFiring(void);
		

		// added by Pete for the use of TargetDisplay
		// -1 is no highlight
		int		getHighlightStyle() const;
		void	setHighlightStyle( int style );

		bool		hasHighlightText() const;
		std::string getHighlightText() const;
		void		setHighlightText( const std::string& styletext );
		bool isTouchProjectileEnabled() {return touchProjectileEnabled;}
		void setTouchProjectileEnabled(bool b) {touchProjectileEnabled = b;}


		bool isShielded() const { return shielded; }
		void setShielded(bool enabled) { shielded = enabled; }
};

}

#endif
