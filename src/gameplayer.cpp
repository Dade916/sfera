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

GamePlayer::GamePlayer(const Properties &prop) :
	gravity(0.f, 0.f, -1.f), material(Spectrum(0.75f, 0.75f, 0.f)) {
	const std::vector<float> vf = Properties::GetParameters(prop, "player.body.position", 3, "0.0 0.0 0.0");
	body.sphere.center = Point(vf[0], vf[1], vf[2]);

	body.sphere.rad = prop.GetFloat("player.body.radius", 1.f);
	body.mass = prop.GetFloat("player.body.mass", 1.f);
	body.staticObject = false;
	body.attractorObject = false;

	viewPhi = 0.f;
	viewTheta = M_PI / 2.f;
}

GamePlayer::~GamePlayer() {
}

void GamePlayer::UpdateCamera(PerspectiveCamera &camera,
		const unsigned int filmWidth, const unsigned int filmHeight) const {
	camera.target = body.sphere.center;
	camera.up = Normalize(-gravity);

	Vector x, y;
	CoordinateSystem(camera.up, &x, &y);
	const Vector dir = SphericalDirection(sinf(viewTheta), cosf(viewTheta), viewPhi, x, y, camera.up);

	camera.orig = camera.target + 5.f * dir;

	camera.Update(filmWidth, filmHeight);
}
