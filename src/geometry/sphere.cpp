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

#include <limits>

#include "geometry/sphere.h"

bool Sphere::Intersect(Ray *ray) const {
	const Vector op = center - ray->o;
	const float b = Dot(op, ray->d);

	float det = b * b - Dot(op, op) + rad * rad;
	if (det < 0.f)
		return false;
	else
		det = sqrtf(det);

	float t = b - det;
	if ((t > ray->mint) && ((t < ray->maxt)))
		ray->maxt = t;
	else {
		t = b + det;

		if ((t > ray->mint) && ((t < ray->maxt)))
			ray->maxt = t;
		else
			return false;
	}

	return true;
}

bool Sphere::IntersectP(const Ray *ray, float *hitT) const {
	const Vector op = center - ray->o;
	const float b = Dot(op, ray->d);

	float det = b * b - Dot(op, op) + rad * rad;
	if (det < 0.f)
		return false;
	else
		det = sqrtf(det);

	float t = b - det;
	if ((t > ray->mint) && ((t < ray->maxt)))
		*hitT = t;
	else {
		t = b + det;

		if ((t > ray->mint) && ((t < ray->maxt)))
			*hitT = t;
		else
			*hitT = std::numeric_limits<float>::infinity();
	}

	return true;
}
