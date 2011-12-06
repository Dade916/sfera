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

#include "sfera.h"
#include "utils/mc.h"
#include "sdl/camera.h"

bool PerspectiveCamera::IsChangedSinceLastUpdate() {
	// Check if the camera parameters have changed since the
	// last check (should be done under mutex)

	if ((DistanceSquared(lastUpdateOrig, orig) < EPSILON) &&
		(DistanceSquared(lastUpdateTarget, target) < EPSILON) &&
		(Dot(lastUpdateUp, up) > .9f)) {
		return false;
	} else {
		lastUpdateOrig = orig;
		lastUpdateTarget = target;
		lastUpdateUp = up;
		return true;
	}
}

void PerspectiveCamera::Update(const unsigned int filmWidth, const unsigned int filmHeight) {
	// Used to move translate the camera
	dir = target - orig;
	dir = Normalize(dir);

	x = Cross(dir, up);
	x = Normalize(x);

	y = Cross(x, dir);
	y = Normalize(y);

	// Used to generate rays
	Transform WorldToCamera = LookAt(orig, target, up);
	CameraToWorld = WorldToCamera.GetInverse();

	Transform CameraToScreen = Perspective(fieldOfView, clipHither, clipYon);

	const float frame =  float(filmWidth) / float(filmHeight);
	float screen[4];
	if (frame < 1.f) {
		screen[0] = -frame;
		screen[1] = frame;
		screen[2] = -1.f;
		screen[3] = 1.f;
	} else {
		screen[0] = -1.f;
		screen[1] = 1.f;
		screen[2] = -1.f / frame;
		screen[3] = 1.f / frame;
	}
	Transform ScreenToRaster =
			Scale(float(filmWidth), float(filmHeight), 1.f) *
			Scale(1.f / (screen[1] - screen[0]), 1.f / (screen[2] - screen[3]), 1.f) *
			::Translate(Vector(-screen[0], -screen[3], 0.f));

	RasterToCamera = CameraToScreen.GetInverse() * ScreenToRaster.GetInverse();
}

void PerspectiveCamera::GenerateRay(
	const float screenX, const float screenY,
	const unsigned int filmWidth, const unsigned int filmHeight,
	Ray *ray, const float u1, const float u2) const {
	Transform c2w = CameraToWorld;

	Point Pras(screenX, filmHeight - screenY - 1.f, 0);
	Point Pcamera;
	RasterToCamera(Pras, &Pcamera);

	ray->o = Pcamera;
	ray->d = Vector(Pcamera.x, Pcamera.y, Pcamera.z);

	// Modify ray for depth of field
	if (lensRadius > 0.f) {
		// Sample point on lens
		float lensU, lensV;
		ConcentricSampleDisk(u1, u2, &lensU, &lensV);
		lensU *= lensRadius;
		lensV *= lensRadius;

		// Compute point on plane of focus
		const float ft = (focalDistance - clipHither) / ray->d.z;
		Point Pfocus = (*ray)(ft);
		// Update ray for effect of lens
		ray->o.x += lensU * (focalDistance - clipHither) / focalDistance;
		ray->o.y += lensV * (focalDistance - clipHither) / focalDistance;
		ray->d = Pfocus - ray->o;
	}

	ray->d = Normalize(ray->d);
	ray->mint = EPSILON;
	ray->maxt = (clipYon - clipHither) / ray->d.z;

	c2w(*ray, ray);
}
