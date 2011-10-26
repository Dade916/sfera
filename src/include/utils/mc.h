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

#ifndef _SFERA_SDL_MC_H
#define	_SFERA_SDL_MC_H

#include <string.h>

#include "sfera.h"

template<class T> inline T Lerp(float t, T v1, T v2) {
	return (1.f - t) * v1 + t * v2;
}

inline void LatLongMappingMap(float s, float t, Vector *wh, float *pdf) {
	const float theta = t * M_PI;
	const float sinTheta = sinf(theta);

	if (wh) {
		const float phi = s * 2.f * M_PI;
		*wh = SphericalDirection(sinTheta, cosf(theta), phi);
	}

	if (pdf)
		*pdf = INV_TWOPI * INV_PI / sinTheta;
}

inline void ConcentricSampleDisk(const float u1, const float u2, float *dx, float *dy) {
	float r, theta;
	// Map uniform random numbers to $[-1,1]^2$
	float sx = 2.f * u1 - 1.f;
	float sy = 2.f * u2 - 1.f;
	// Map square to $(r,\theta)$
	// Handle degeneracy at the origin
	if (sx == 0.f && sy == 0.f) {
		*dx = 0.f;
		*dy = 0.f;
		return;
	}
	if (sx >= -sy) {
		if (sx > sy) {
			// Handle first region of disk
			r = sx;
			if (sy > 0.f)
				theta = sy / r;
			else
				theta = 8.f + sy / r;
		} else {
			// Handle second region of disk
			r = sy;
			theta = 2.f - sx / r;
		}
	} else {
		if (sx <= sy) {
			// Handle third region of disk
			r = -sx;
			theta = 4.f - sy / r;
		} else {
			// Handle fourth region of disk
			r = -sy;
			theta = 6.f + sx / r;
		}
	}
	theta *= M_PI / 4.f;
	*dx = r * cosf(theta);
	*dy = r * sinf(theta);
}

inline Vector UniformSampleSphere(const float u1, const float u2) {
	float z = 1.f - 2.f * u1;
	float r = sqrtf(Max(0.f, 1.f - z * z));
	float phi = 2.f * M_PI * u2;
	float x = r * cosf(phi);
	float y = r * sinf(phi);

	return Vector(x, y, z);
}

inline Vector CosineSampleHemisphere(const float u1, const float u2) {
	Vector ret;
	ConcentricSampleDisk(u1, u2, &ret.x, &ret.y);
	ret.z = sqrtf(Max(0.f, 1.f - ret.x * ret.x - ret.y * ret.y));

	return ret;
}

inline Vector UniformSampleCone(float u1, float u2, float costhetamax) {
	float costheta = Lerp(u1, costhetamax, 1.f);
	float sintheta = sqrtf(1.f - costheta*costheta);
	float phi = u2 * 2.f * M_PI;
	return Vector(cosf(phi) * sintheta,
	              sinf(phi) * sintheta,
		          costheta);
}

inline Vector UniformSampleCone(float u1, float u2, float costhetamax, const Vector &x, const Vector &y, const Vector &z) {
	float costheta = Lerp(u1, costhetamax, 1.f);
	float sintheta = sqrtf(1.f - costheta*costheta);
	float phi = u2 * 2.f * M_PI;
	return cosf(phi) * sintheta * x + sinf(phi) * sintheta * y + costheta * z;
}

inline float UniformConePdf(float costhetamax) {
	return 1.f / (2.f * M_PI * (1.f - costhetamax));
}

#endif	/* _SFERA_SDL_MC_H */
