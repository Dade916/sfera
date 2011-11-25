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

#ifndef _SFERA_SDL_CAMERA_H
#define	_SFERA_SDL_CAMERA_H

#include <limits>

#include "geometry/transform.h"

class PerspectiveCamera {
public:
	PerspectiveCamera() { }
	PerspectiveCamera(const Point &o, const Point &t, const Vector &u) :
		orig(o), target(t), up(Normalize(u)), fieldOfView(45.f), clipHither(1e-3f), clipYon(1e30f),
		lensRadius(0.f), focalDistance(10.f) {
		const float nan = std::numeric_limits<float>::quiet_NaN();
		lastUpdateOrig = Point(nan, nan, nan);
		lastUpdateTarget = Point(nan, nan, nan);
		lastUpdateUp = Vector(nan, nan, nan);
	}

	~PerspectiveCamera() { }

	void TranslateLeft(const float k) {
		Vector t = -k * Normalize(x);
		Translate(t);
	}

	void TranslateRight(const float k) {
		Vector t = k * Normalize(x);
		Translate(t);
	}

	void TranslateForward(const float k) {
		Vector t = k * dir;
		Translate(t);
	}

	void TranslateBackward(const float k) {
		Vector t = -k * dir;
		Translate(t);
	}

	void Translate(const Vector &t) {
		orig += t;
		target += t;
	}

	void RotateLeft(const float k) {
		Rotate(k, y);
	}

	void RotateRight(const float k) {
		Rotate(-k, y);
	}

	void RotateUp(const float k) {
		Rotate(k, x);
	}

	void RotateDown(const float k) {
		Rotate(-k, x);
	}

	void Rotate(const float angle, const Vector &axis) {
		Vector p = target - orig;
		Transform t = ::Rotate(angle, axis);
		target = orig + t(p);
	}

	void Update(const unsigned int filmWidth, const unsigned int filmHeight);
	bool IsChangedSinceLastUpdate();

	void GenerateRay(
		const float screenX, const float screenY,
		const unsigned int filmWidth, const unsigned int filmHeight,
		Ray *ray, const float u1, const float u2) const;

	const Matrix4x4 GetRasterToCameraMatrix() const {
		return RasterToCamera.GetMatrix();
	}

	const Matrix4x4 GetCameraToWorldMatrix() const {
		return CameraToWorld.GetMatrix();
	}

	float GetClipYon() const { return clipYon; }
	float GetClipHither() const { return clipHither; }
	float GetLensRadius() const { return lensRadius; }
	float GetFocalDistance() const { return focalDistance; }

	// User defined values
	Point orig, target;
	Vector up;

private:
	Point lastUpdateOrig, lastUpdateTarget;
	Vector lastUpdateUp;

	float fieldOfView, clipHither, clipYon, lensRadius, focalDistance;

	// Calculated values
	Vector dir, x, y;
	Transform RasterToCamera, CameraToWorld;
};

#endif	/* _SFERA_SDL_CAMERA_H */
