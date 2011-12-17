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

#ifndef _SFERA_GAMEPLAYER_H
#define	_SFERA_GAMEPLAYER_H

#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#include "sfera.h"
#include "utils/properties.h"
#include "gamesphere.h"
#include "sdl/material.h"
#include "sdl/camera.h"

#define GAMEPLAYER_PUPPET_SIZE 6

class GamePlayer {
public:
	GamePlayer(const Properties &prop);
	~GamePlayer();

	void UpdateLocalCoordSystem();
	void ApplyInputs(const float refreshRate);
	void UpdatePuppet();
	void UpdateCamera(PerspectiveCamera &camera,
		const unsigned int filmWidth, const unsigned int filmHeight) const;

	void SetGravity(const Vector g) { gravity = g; up = Normalize(-gravity);}

	float forwardSpeed;
	float jumpSpeed;

	GameSphere body;

	Vector gravity; // Physic engine writes, other read
	Vector up, front, right; // up is normalized -gravity vector

	Sphere puppet[GAMEPLAYER_PUPPET_SIZE];
	MatteMaterial *puppetMaterial[GAMEPLAYER_PUPPET_SIZE];

	// Camera position
	float targetPhi, targetTheta, viewPhi, viewTheta, viewDistance; // User input write, other read
	bool targetPuppet;

	// Control
	bool inputGoForward, inputTurnLeft, inputTurnRight, inputSlowDown;
	bool inputJump; // Set by user input, reset by physic engine
};

#endif	/* _SFERA_GAMEPLAYER_H */
