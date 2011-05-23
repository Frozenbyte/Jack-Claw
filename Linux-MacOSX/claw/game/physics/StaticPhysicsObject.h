
#ifndef STATICPHYSICSOBJECT_H
#define STATICPHYSICSOBJECT_H

#include "AbstractPhysicsObject.h"
#include "DatatypeDef.h"
#include <boost/shared_ptr.hpp>
#include "physics_collisiongroups.h"

class IStorm3D_Model;

namespace frozenbyte {
namespace physics {

class StaticMesh;

} // physics
} // frozenbyte


namespace game
{
	class GamePhysics;
	class GamePhysicsImpl;
	class StaticPhysicsObjectImpl;

	class StaticPhysicsObject : public AbstractPhysicsObject
	{
	public:
		StaticPhysicsObject(GamePhysics *gamePhysics, const char *filename, IStorm3D_Model *model, const VC3 &position, const QUAT &rotation, int collisionGroup = PHYSICS_COLLISIONGROUP_STATIC);
#ifdef PHYSICS_PHYSX
		StaticPhysicsObject(GamePhysics *gamePhysics, boost::shared_ptr<frozenbyte::physics::StaticMesh> &mesh, const VC3 &position, const QUAT &rotation, int collisionGroup = PHYSICS_COLLISIONGROUP_STATIC);
#endif

		virtual ~StaticPhysicsObject();

	protected:
		virtual PHYSICS_ACTOR createImplementationObject();

		virtual void syncImplementationObject(PHYSICS_ACTOR &obj);

		virtual void syncInactiveImplementationObject(PHYSICS_ACTOR &obj);

	private:
		StaticPhysicsObjectImpl *impl;

		static void clearImplementationResources();

		friend class GamePhysicsImpl;

		int collisionGroup;
	};
}

#endif

