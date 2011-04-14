
#include "precompiled.h"

#include "UnifiedHandleManager.h"
#include "unified_handle.h"
#include "Game.h"
#include "GameUI.h"
#include "UnitList.h"
#include "UnitActor.h"
#include "unittypes.h"
#include "../ui/LightManager.h"
#include "../ui/DynamicLightManager.h"
#include "ParticleSpawnerManager.h"
#include "tracking/ObjectTracker.h"
#include "tracking/ITrackerObject.h"

using namespace game::tracking;

namespace game
{

	UnifiedHandleManager::UnifiedHandleManager(Game *game)
	{
		this->game = game;
	}

	UnifiedHandleManager::~UnifiedHandleManager()
	{
		// nop
	}

	bool UnifiedHandleManager::doesObjectExist(UnifiedHandle uh) const
	{
		if (IS_UNIFIED_HANDLE_UNIT(uh))
		{
			return game->units->doesTrackableUnifiedHandleObjectExist(uh);
		}
		else if (IS_UNIFIED_HANDLE_TERRAIN_OBJECT(uh))
		{
			if (game->gameUI->getTerrain())
				return game->gameUI->getTerrain()->doesTrackableUnifiedHandleObjectExist(uh);
			else
				return false;
		}
		else if (IS_UNIFIED_HANDLE_LIGHT(uh))
		{
			if (game->gameUI->getLightManager())
				return game->gameUI->getLightManager()->doesLightExist(uh);
			else
				return false;
		}
		else if (IS_UNIFIED_HANDLE_DYNAMIC_LIGHT(uh))
		{
			if (game->gameUI->getDynamicLightManager())
				return game->gameUI->getDynamicLightManager()->doesLightExist(uh);
			else
				return false;
		}
		else if (IS_UNIFIED_HANDLE_TRACKER(uh))
		{
			ITrackerObject *t = game->objectTracker->getTrackerByUnifiedHandle(uh);
			if (t != NULL)
			{
				return true;
			} else {
				return false;
			}
		}
		else if (IS_UNIFIED_HANDLE_PARTICLE_SPAWNER(uh))
		{
			if (game->particleSpawnerManager)
				return game->particleSpawnerManager->doesParticleSpawnerExist(uh);
			else
				return false;
		}
		else 
		{
			// TODO: other types.
			assert(!"UnifiedHandleManager::doesObjectExist - TODO, other types.");
			return false;
		}
	}

	VC3 UnifiedHandleManager::getObjectPosition(UnifiedHandle uh) const
	{
		if (IS_UNIFIED_HANDLE_UNIT(uh))
		{
			assert(game->units->doesTrackableUnifiedHandleObjectExist(uh));
			return game->units->getTrackableUnifiedHandlePosition(uh);
		}
		else if (IS_UNIFIED_HANDLE_TERRAIN_OBJECT(uh))
		{
			assert(game->gameUI->getTerrain()->doesTrackableUnifiedHandleObjectExist(uh));
			return game->gameUI->getTerrain()->getTrackableUnifiedHandlePosition(uh);
		}
		else if (IS_UNIFIED_HANDLE_LIGHT(uh))
		{
			assert(game->gameUI->getLightManager()->doesLightExist(uh));
			return game->gameUI->getLightManager()->getLightPosition(uh);
		}
		else if (IS_UNIFIED_HANDLE_DYNAMIC_LIGHT(uh))
		{
			assert(game->gameUI->getDynamicLightManager()->doesLightExist(uh));
			return game->gameUI->getDynamicLightManager()->getLightPosition(uh);
		}
		else if (IS_UNIFIED_HANDLE_TRACKER(uh))
		{
			ITrackerObject *t = game->objectTracker->getTrackerByUnifiedHandle(uh);
			assert(t != NULL);
			return t->getTrackerPosition();
		}
		else if (IS_UNIFIED_HANDLE_PARTICLE_SPAWNER(uh))
		{
			assert(game->particleSpawnerManager->doesParticleSpawnerExist(uh));
			return game->particleSpawnerManager->getParticleSpawnerPosition(uh);
		}
		else 
		{
			// TODO: other types.
			assert(!"UnifiedHandleManager::getObjectPosition - TODO, other types.");
			return VC3(0,0,0);
		}
	}

	void UnifiedHandleManager::setObjectPosition(UnifiedHandle uh, const VC3 &position)
	{
		if (IS_UNIFIED_HANDLE_TRACKER(uh))
		{
			ITrackerObject *t = game->objectTracker->getTrackerByUnifiedHandle(uh);
			assert(t != NULL);
			t->setTrackerPosition(position);
		}
		else if (IS_UNIFIED_HANDLE_UNIT(uh))
		{
			assert(game->units->doesTrackableUnifiedHandleObjectExist(uh));
			Unit *u = game->units->getUnitById(game->units->unifiedHandleToUnitId(uh));
			UnitActor *ua = getUnitActorForUnit(u);
			assert(ua != NULL);
			ua->removeUnitObstacle(u);
			u->setPosition(position);
			ua->addUnitObstacle(u);
		}
		else if (IS_UNIFIED_HANDLE_TERRAIN_OBJECT(uh))
		{
			assert(game->gameUI->getTerrain()->doesTrackableUnifiedHandleObjectExist(uh));
			int modelId = 0;
			int instanceId = 0;
			game->gameUI->getTerrain()->unifiedHandleToTerrainIds(uh, &modelId, &instanceId);
			game->gameUI->getTerrain()->setTerrainObjectPosition(modelId, instanceId, position);
		}
		else if (IS_UNIFIED_HANDLE_LIGHT(uh))
		{
			assert(game->gameUI->getLightManager()->doesLightExist(uh));
			game->gameUI->getLightManager()->setLightPositionByUnifiedHandle(uh, position);
		}
		else if (IS_UNIFIED_HANDLE_DYNAMIC_LIGHT(uh))
		{
			assert(game->gameUI->getDynamicLightManager()->doesLightExist(uh));
			game->gameUI->getDynamicLightManager()->setLightPositionByUnifiedHandle(uh, position);
		}
		else if (IS_UNIFIED_HANDLE_PARTICLE_SPAWNER(uh))
		{
			assert(game->particleSpawnerManager->doesParticleSpawnerExist(uh));
			game->particleSpawnerManager->setParticleSpawnerPosition(uh, position);
		}
		else 
		{
			// TODO: other types.
			assert(!"UnifiedHandleManager::setObjectPosition - TODO, other types.");
		}
	}

	QUAT UnifiedHandleManager::getObjectRotation(UnifiedHandle uh) const
	{
		// TODO
		assert(!"UnifiedHandleManager::getObjectRotation - TODO.");
		return QUAT();
	}

	void UnifiedHandleManager::setObjectRotation(UnifiedHandle uh, const QUAT &rotation)
	{
		// TODO
		assert(!"UnifiedHandleManager::setObjectRotation - TODO.");
	}

	VC3 UnifiedHandleManager::getObjectVelocity(UnifiedHandle uh) const
	{
		// TODO
		assert(!"UnifiedHandleManager::getObjectVelocity - TODO.");
		return VC3(0,0,0);
	}

	void UnifiedHandleManager::setObjectVelocity(UnifiedHandle uh, const VC3 &velocity)
	{
		// TODO
		assert(!"UnifiedHandleManager::setObjectVelocity - TODO.");
	}

	VC3 UnifiedHandleManager::getObjectCenterPosition(UnifiedHandle uh) const
	{
		// TODO
		//assert(!"UnifiedHandleManager::getObjectCenterPosition - TODO.");
		return getObjectPosition(uh);
	}

	void UnifiedHandleManager::setObjectCenterPosition(UnifiedHandle uh, const VC3 &centerPosition)
	{
		// TODO
		//assert(!"UnifiedHandleManager::setObjectCenterPosition - TODO.");
		return setObjectPosition(uh, centerPosition);
	}

}
