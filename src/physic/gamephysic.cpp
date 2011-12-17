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

#include "physic/gamephysic.h"

//------------------------------------------------------------------------------
// GamePhysic
//------------------------------------------------------------------------------

GamePhysic::GamePhysic(GameLevel *level) : gameLevel(level) {
	runningHz = gameLevel->gameConfig->GetPhysicRefreshRate();

	// Initialize Bullet Physics Engine
	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver();

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase,
			solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0.f, 0.f , -10.f));

	// Add all spheres
	vector<GameSphere> &gameSpheres(gameLevel->scene->spheres);
	for (size_t s = 0; s < gameSpheres.size(); ++s)
		AddRigidBody(gameSpheres[s], s);

	// Add the player body
	AddRigidBody(gameLevel->player->body, 0);
}

GamePhysic::~GamePhysic() {
	for (size_t i = 0; i < staticRigidBodies.size(); ++i)
		DeleteRigidBody(staticRigidBodies[i]);

	for (size_t i = 0; i < dynamicRigidBodies.size(); ++i)
		DeleteRigidBody(dynamicRigidBodies[i]);

	delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
}

void GamePhysic::AddRigidBody(GameSphere &gameSphere, const size_t index) {
	btCollisionShape *shape = new btSphereShape(gameSphere.sphere.rad);
	btVector3 inertia(0.f, 0.f, 0.f);
	float mass = gameSphere.staticObject ? 0.f : gameSphere.mass;
	shape->calculateLocalInertia(mass, inertia);

	btDefaultMotionState *motionState = new btDefaultMotionState(btTransform(btQuaternion(0.f, 0.f, 0.f, 1.f),
				btVector3(gameSphere.sphere.center.x, gameSphere.sphere.center.y, gameSphere.sphere.center.z)));

	btRigidBody::btRigidBodyConstructionInfo
			groundRigidBodyCI(mass, motionState, shape, inertia);
	groundRigidBodyCI.m_restitution = 0.25f;

	btRigidBody *rigidBody = new btRigidBody(groundRigidBodyCI);
	dynamicsWorld->addRigidBody(rigidBody);

	if (gameSphere.staticObject)
		staticRigidBodies.push_back(rigidBody);
	else {
		rigidBody->setActivationState(DISABLE_DEACTIVATION);
		rigidBody->setDamping(gameSphere.linearDamping, gameSphere.angularDamping);

		dynamicRigidBodies.push_back(rigidBody);
		dynamicRigidBodyIndices.push_back(index);
	}

	if (gameSphere.attractorObject)
		gravitySphereIndices.push_back(index);

	rigidBody->setUserPointer((void *)&gameSphere);
}

void GamePhysic::DeleteRigidBody(btRigidBody *rigidBody) {
	dynamicsWorld->removeRigidBody(rigidBody);
	delete rigidBody->getMotionState();
	delete rigidBody->getCollisionShape();
	delete rigidBody;
}

void GamePhysic::UpdateGameSphere(GameSphere &gameSphere, btRigidBody *dynamicRigidBody) {
	btTransform trans;
	dynamicRigidBody->getMotionState()->getWorldTransform(trans);

	Sphere &sphere(gameSphere.sphere);
	const btVector3 &orig = trans.getOrigin();
	sphere.center.x = orig.getX();
	sphere.center.y = orig.getY();
	sphere.center.z = orig.getZ();

	// Update gravity
	vector<GameSphere> &gameSpheres(gameLevel->scene->spheres);
	const float gravityConstant = gameLevel->scene->gravityConstant;
	Vector F;
	for (size_t j = 0; j < gravitySphereIndices.size(); ++j) {
		GameSphere &gamePlanet(gameSpheres[gravitySphereIndices[j]]);
		Vector dir = gamePlanet.sphere.center - gameSphere.sphere.center;
		const float distance2 = dir.LengthSquared();
		dir /= sqrtf(distance2);

		F += dir * (gravityConstant * gameSphere.mass * gamePlanet.mass / distance2);
	}

	const Vector a = F / gameSphere.mass;
	dynamicRigidBody->setGravity(btVector3(a.x, a.y, a.z));
}

void GamePhysic::DoStep() {
	boost::unique_lock<boost::mutex> lock(gameLevel->levelMutex);

	// Update the gravity of all dynamic spheres
	vector<GameSphere> &gameSpheres(gameLevel->scene->spheres);
	for (size_t i = 0; i < dynamicRigidBodies.size() - 1; ++i)
		UpdateGameSphere(gameSpheres[dynamicRigidBodyIndices[i]], dynamicRigidBodies[i]);

	// Update the player
	GamePlayer &player(*(gameLevel->player));
	btRigidBody *playerRigidBody = dynamicRigidBodies[dynamicRigidBodies.size() - 1];
	UpdateGameSphere(player.body, playerRigidBody);
	const btVector3 &v = playerRigidBody->getGravity();
	player.SetGravity(Vector(v.getX(), v.getY(), v.getZ()));

	// Apply user inputs
	if (player.inputGoForward) {
		const Vector forward =  player.forwardSpeed * player.front;

		playerRigidBody->applyCentralForce(btVector3(
			forward.x,
			forward.y,
			forward.z));
	} else
		playerRigidBody->clearForces();

	if (player.inputSlowDown)
		playerRigidBody->setDamping(
			10.f * player.body.linearDamping,
			10.f * player.body.angularDamping);
	else
		playerRigidBody->setDamping(
			player.body.linearDamping,
			player.body.angularDamping);

	if (player.inputJump) {
		player.inputJump = false;
		const Vector jump =  player.jumpSpeed * player.up;

		playerRigidBody->applyCentralImpulse(btVector3(
			jump.x,
			jump.y,
			jump.z));
	}

	dynamicsWorld->stepSimulation(1.f / gameLevel->gameConfig->GetPhysicRefreshRate(), 4);

	gameLevel->Refresh(gameLevel->gameConfig->GetPhysicRefreshRate());

	// Check if one of the pills was hit
	int numManifolds = dispatcher->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		btPersistentManifold *contactManifold = dispatcher->getManifoldByIndexInternal(i);
		btCollisionObject *objA = (btCollisionObject*)(contactManifold->getBody0());
		btCollisionObject *objB = (btCollisionObject*)(contactManifold->getBody1());

		int numContacts = contactManifold->getNumContacts();
		if (numContacts > 0) {
			GameSphere *sphereA = (GameSphere *)objA->getUserPointer();
			GameSphere *sphereB = (GameSphere *)objB->getUserPointer();
			if (sphereA->puppetObject)
				Swap(sphereA, sphereB);

			if (sphereA->pillObject && sphereB->puppetObject && !sphereA->isPillOff) {
				// Disable the pill
				sphereA->isPillOff = true;
				++(gameLevel->offPillCount);
				// Use the off material for this pill
				gameLevel->scene->sphereMaterials[sphereA->index] = gameLevel->scene->pillOffMaterial;
				gameLevel->editActionList.AddAction(MATERIALS_EDIT);
			}
		}
	}

	gameLevel->editActionList.AddAction(CAMERA_EDIT);
	gameLevel->editActionList.AddAction(GEOMETRY_EDIT);
}

//------------------------------------------------------------------------------
// PhysicThread
//------------------------------------------------------------------------------

PhysicThread::PhysicThread(GamePhysic *physic) : gamePhysic(physic), physicThread(NULL) {
}

PhysicThread::~PhysicThread() {
	if (physicThread)
		Stop();
}

void PhysicThread::Start() {
	physicThread = new boost::thread(boost::bind(PhysicThread::PhysicThreadImpl, this));
}

void PhysicThread::Stop() {
	if (physicThread) {
		physicThread->interrupt();
		physicThread->join();
		delete physicThread;
		physicThread = NULL;
	}
}

void PhysicThread::PhysicThreadImpl(PhysicThread *physicThread) {
	try {
		unsigned int frame = 0;
		double frameStartTime = WallClockTime();
		const double refreshTime = 1.0 / physicThread->gamePhysic->gameLevel->gameConfig->GetPhysicRefreshRate();
		double currentRefreshTime = 0.0;

		while (!boost::this_thread::interruption_requested()) {
			const double t1 = WallClockTime();
			physicThread->gamePhysic->DoStep();
			const double t2 = WallClockTime();
			currentRefreshTime = t2 - t1;

			if (currentRefreshTime < refreshTime) {
				const unsigned int sleep = (unsigned int)((refreshTime - currentRefreshTime) * 1000.0);
				if (sleep > 0)
					boost::this_thread::sleep(boost::posix_time::millisec(sleep));
			}

			++frame;
			if (frame == 240) {
				const double now = WallClockTime();
				physicThread->gamePhysic->runningHz = 240.0 / (now - frameStartTime);

				frameStartTime = now;
				frame = 0;
			}
		}
	} catch (boost::thread_interrupted) {
		SFERA_LOG("[PhysicThread] Physic thread halted");
	}
}
