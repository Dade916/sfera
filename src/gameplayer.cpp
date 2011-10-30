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

#include "gameplayer.h"
#include "gamesphere.h"
#include "geometry/vector_normal.h"

GamePlayer::GamePlayer(const Properties &prop)  {
	const std::vector<float> vf = Properties::GetParameters(prop, "player.body.position", 3, "0.0 0.0 0.0");
	body.sphere.center = Point(vf[0], vf[1], vf[2]);

	body.sphere.rad = prop.GetFloat("player.body.radius", 1.f);
	body.mass = prop.GetFloat("player.body.mass", 1.f);
	body.staticObject = false;
	body.attractorObject = false;

	viewPhi = 0.f;
	viewTheta = -M_PI / 2.f;
	viewDistance = body.sphere.rad * 20.f;

	inputGoForward = false;
	inputTurnLeft = false;
	inputTurnRight = false;

	front = Vector(0.f, 1.f, 0.f);
			
	puppetMaterial[0] = new MatteMaterial(Spectrum(0.75f, 0.75f, 0.f));
	puppetMaterial[1] = new MatteMaterial(Spectrum(0.75f, 0.75f, 0.75f));
	puppetMaterial[2] = new MatteMaterial(Spectrum(0.05f, 0.05f, 0.05f));
	puppetMaterial[3] = new MatteMaterial(Spectrum(0.75f, 0.75f, 0.75f));
	puppetMaterial[4] = new MatteMaterial(Spectrum(0.05f, 0.05f, 0.05f));
	puppetMaterial[5] = new MatteMaterial(Spectrum(0.75f, 0.f, 0.f));

	SetGravity(Vector(0.f, 0.f, -1.f));
	UpdateLocalCoordSystem();
	UpdatePuppet();
}

GamePlayer::~GamePlayer() {
}

void GamePlayer::UpdateCamera(PerspectiveCamera &camera,
		const unsigned int filmWidth, const unsigned int filmHeight) const {
	camera.target = body.sphere.center;

	const Vector dir = SphericalDirection(sinf(viewTheta), cosf(viewTheta), viewPhi, front, right, up);
	if (AbsDot(dir, camera.up) < 1.f - EPSILON) {
		camera.orig = camera.target + viewDistance * dir;
		camera.up = up;

		camera.Update(filmWidth, filmHeight);
	}
}

void GamePlayer::ApplyInputs() {
	// TODO: movements should be not refresh rate related
	if (inputTurnLeft) {
		// Rotate left
		Transform t = Rotate(10.f, up);
		front = t(front);
	}

	if (inputTurnRight) {
		// Rotate right
		Transform t = Rotate(-10.f, up);
		front = t(front);
	}
}

void GamePlayer::UpdatePuppet() {
	const float scale = body.sphere.rad;

	// Body
	puppet[0] = body.sphere;

	// Left eye
	puppet[1].center = body.sphere.center + scale * (0.6f * up - 0.4f * right + 0.4f * front);
	puppet[1].rad = scale * 0.3f;
	puppet[2].center = body.sphere.center + scale * (0.6f * up - 0.4f * right + 0.55f * front);
	puppet[2].rad = scale * 0.2f;

	// Right eye
	puppet[3].center = body.sphere.center + scale * (0.6f * up + 0.4f * right + 0.4f * front);
	puppet[3].rad = scale * 0.3f;
	puppet[4].center = body.sphere.center + scale * (0.6f * up + 0.4f * right + 0.55f * front);
	puppet[4].rad = scale * 0.2f;

	// Nose
	puppet[5].center = body.sphere.center + scale * 1.1f * front;
	puppet[5].rad = scale * 0.3f;
}
