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

GamePhysic::GamePhysic(GameLevel *level) : gameLevel(level) {
	// Initialize Bullet Physics Engine
	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver();

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase,
			solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0.f, 0.f , -10.f));

	// Add all spheres
	const vector<GameSphere> &gameSpheres(gameLevel->scene->spheres);
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

void GamePhysic::AddRigidBody(const GameSphere &gameSphere, const size_t index) {
	btCollisionShape *shape = new btSphereShape(gameSphere.sphere.rad);
	btVector3 inertia(0.f, 0.f, 0.f);
	shape->calculateLocalInertia(gameSphere.mass, inertia);

	btDefaultMotionState *motionState = new btDefaultMotionState(btTransform(btQuaternion(0.f, 0.f, 0.f, 1.f),
				btVector3(gameSphere.sphere.center.x, gameSphere.sphere.center.y, gameSphere.sphere.center.z)));

	btRigidBody::btRigidBodyConstructionInfo
			groundRigidBodyCI(gameSphere.mass, motionState, shape, inertia);

	btRigidBody *rigidBody = new btRigidBody(groundRigidBodyCI);
	dynamicsWorld->addRigidBody(rigidBody);

	if (gameSphere.mass > 0.f) {
		dynamicRigidBodies.push_back(rigidBody);
		dynamicRigidBodyIndices.push_back(index);
	} else
		staticRigidBodies.push_back(rigidBody);
}

void GamePhysic::DeleteRigidBody(btRigidBody *rigidBody) {
	dynamicsWorld->removeRigidBody(rigidBody);
	delete rigidBody->getMotionState();
	delete rigidBody->getCollisionShape();
	delete rigidBody;
}

void GamePhysic::DoStep() {
	dynamicsWorld->stepSimulation(1 / 60.f, 10.f);

	// Update the position of all dynamic spheres
	vector<GameSphere> &gameSpheres(gameLevel->scene->spheres);
	for (size_t i = 0; i < dynamicRigidBodies.size() - 1; ++i) {
		btTransform trans;
		dynamicRigidBodies[i]->getMotionState()->getWorldTransform(trans);

		Sphere &sphere(gameSpheres[dynamicRigidBodyIndices[i]].sphere);
		const btVector3 &orig = trans.getOrigin();
		sphere.center.x = orig.getX();
		sphere.center.y = orig.getY();
		sphere.center.z = orig.getZ();
	}

	// Update the player
	btTransform trans;
	dynamicRigidBodies[dynamicRigidBodies.size() - 1]->getMotionState()->getWorldTransform(trans);

	Sphere &sphere(gameLevel->player->body.sphere);
	const btVector3 &orig = trans.getOrigin();
	sphere.center.x = orig.getX();
	sphere.center.y = orig.getY();
	sphere.center.z = orig.getZ();
}
