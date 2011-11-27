/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of Sfera.                                           *
 *                                                                         *
 *   Sfera is free software; you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Sfera is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef _SFERA_GAMEPHYSIC_H
#define	_SFERA_GAMEPHYSIC_H

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <boost/thread/thread.hpp>

#include "sfera.h"
#include "gamelevel.h"

#define PHYSIC_DEFAULT_ANGULAR_DAMPING 0.5f
#define PHYSIC_DEFAULT_LINEAR_DAMPING 0.05f

class GamePhysic {
public:
	GamePhysic(GameLevel *level);
	~GamePhysic();

	double GetRunningHz() const { return runningHz; }

	void DoStep();

	GameLevel *gameLevel;

	friend class PhysicThread;

private:
	void UpdateGameSphere(GameSphere &gameSphere, btRigidBody *dynamicRigidBody);
	void AddRigidBody(GameSphere &gameSphere, const size_t index = 0);
	void DeleteRigidBody(btRigidBody *rigidBody);

	btBroadphaseInterface *broadphase;
	btDefaultCollisionConfiguration *collisionConfiguration;
	btCollisionDispatcher *dispatcher;
	btSequentialImpulseConstraintSolver *solver;

	btDiscreteDynamicsWorld *dynamicsWorld;

	vector<btRigidBody *> staticRigidBodies;

	// NOTE: the last one is always the player GameSphere
	vector<btRigidBody *> dynamicRigidBodies;
	vector<size_t> dynamicRigidBodyIndices;

	vector<size_t> gravitySphereIndices;

	double runningHz;
};

class PhysicThread {
public:
	PhysicThread(GamePhysic *physic);
	~PhysicThread();

	void Start();
	void Stop();

	GamePhysic *gamePhysic;

private:
	static void PhysicThreadImpl(PhysicThread *physicThread);

	boost::thread *physicThread;
};

#endif	/* _SFERA_GAMEPHYSIC_H */
